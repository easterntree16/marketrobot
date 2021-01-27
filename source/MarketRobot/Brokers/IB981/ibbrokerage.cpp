#include "Brokers/IB981/ibbrokerage.h"
#include "Common/Data/datatype.h"
#include "Common/Data/datamanager.h"
#include "DataCenter/datacenter.h"
#include "Common/Order/orderstatus.h"
#include "Common/Order/ordermanager.h"
#include "Common/Util/util.h"
#include "Common/Security/portfoliomanager.h"
#include "Common/Logger/spdlogger.h"

#include <mutex>
#include <algorithm>
#include <thread>
#include <chrono>

using namespace std;
using namespace MarketRobot;

// http://interactivebrokers.github.io/tws-api/
namespace MarketRobot
{
	
	extern std::atomic<bool> gShutdown;
	
	IBBrokerage::IBBrokerage() :
		m_osSignal(2000)//2-seconds timeout
		, m_pClient(new ::EClientSocket(this, &m_osSignal))
		, m_sleepDeadline(0)
		, m_pReader(0)
		, m_extraAuth(false)
		, lastPriceCache_(CConfig::instance().securities.size(), 0.0)
		, bidPriceCache_(CConfig::instance().securities.size(), 0.0)
		, askPriceCache_(CConfig::instance().securities.size(), 0.0)
	{
	}

	//! [socket_init]
	IBBrokerage::~IBBrokerage()
	{
		if (m_pReader)
			delete m_pReader;
		m_pClient->eDisconnect();
		delete m_pClient;
	}

	//********************************************************************************************//
	// Brokerage part
	void IBBrokerage::processBrokerageMessages()
	{
		fd_set readSet, writeSet, errorSet;

		struct timeval tval;
		tval.tv_usec = 0;
		tval.tv_sec = 0;

		time_t now = std::time(NULL);

		if (!brokerage::heatbeat(5)) {
			disconnectFromBrokerage();
			return;
		}

		switch (bkstate_) {
		case BK_ACCOUNT:		// not used
			requestBrokerageAccountInformation(CConfig::instance().account);
			break;
		case BK_ACCOUNTACK:		// not used
			break;
		case BK_GETORDERID:
			requestNextValidOrderID();
			break;
		case BK_GETORDERIDACK:
			break;
		case BK_READYTOORDER:
			monitorClientRequest();
			break;
		case BK_PLACEORDER_ACK:
			break;
		case BK_CANCELORDER:
			cancelOrder(0);
			break;
		case BK_CANCELORDER_ACK:
			break;
		}

		if (m_sleepDeadline > 0) {
			// initialize timeout with m_sleepDeadline - now
			tval.tv_sec = m_sleepDeadline - now;
		}
		m_osSignal.waitForSignal();
		m_pReader->processMsgs();
	}

	bool IBBrokerage::connectToBrokerage() {
		const char* host = CConfig::instance().ib_host.c_str();
		auto port = CConfig::instance().ib_port;
		int clientId = 0;

		LOG("Connecting to {}:{} clientId:{}.", host, port, clientId);
		//! [connect]
		bool bRes = m_pClient->eConnect(host, port, clientId, m_extraAuth);
		//! [connect]
		while (!m_pClient->isConnected()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		DEBUG("m_pClient->isConnected={}", m_pClient->isConnected());

		if (bRes) {
			LOG("Connected to ib brokerage {}:{} clientId:{}", host, port, clientId);
			//! [ereader]
			m_pReader = new ::EReader(m_pClient, &m_osSignal);
			m_pReader->start();
			//! [ereader]
			bkstate_ = BK_CONNECTED;
			//m_pClient->setServerLogLevel(5);			// can not work on m_pClient before a loop process
			if (clientId == 0) {
				m_pClient->reqAllOpenOrders();		// associate TWS with the client
			}
			//_nServerVersion = m_pClient->serverVersion();
			// trigger updatePortfolio()
			//m_pClient->reqAccountUpdates();
		}
		else {
			LOG_ERROR("Cannot connect to {}:{} clientId:{}",host, port, clientId);
		}
		return bRes;
	}

	void IBBrokerage::disconnectFromBrokerage() {
		// CancelMarketData
		int i = 0;
		for (auto it = CConfig::instance().securities.begin(); it != CConfig::instance().securities.end(); ++it)
		{
			m_pClient->cancelMktData(i);
			i++;
		}

		m_pClient->eDisconnect();
		bkstate_ = BK_DISCONNECTED;
		LOG_INFO("TWS connection disconnected!");
	}

	bool IBBrokerage::isConnectedToBrokerage() const
	{
		
		//DEBUG("m_pClient->isConnected={},bkstate_={}", m_pClient->isConnected() , bkstate_);
		
		return (m_pClient->isConnected() && (bkstate_ >= BK_CONNECTED));
	}

	void IBBrokerage::placeOrder(std::shared_ptr<MarketRobot::Order> o)
	{
		LOG_INFO("Place order, id = {}",(long)o->serverOrderId);

		::Order oib;			// local stack
		Contract contract;				// local stack

		if (o->fullSymbol.empty())
		{
			ERROR("Order is not valid {}",(long)o->serverOrderId);
			return;
		}

		if (o->serverOrderId < 0)
		{
			ERROR("Order is not valid {}",(long)o->serverOrderId);
			return;
		}

		if (o->orderStatus != OrderStatus::OS_NewBorn)		// in order to enable replacement order; this part needs to be commented out
		{
			ERROR("Not a NewBorn order {}",(long)o->serverOrderId);
			return;
		}

		SecurityFullNameToContract(o->fullSymbol, contract);
		OrderToIBOfficialOrder(o, oib);

		lock_guard<mutex> g(orderStatus_mtx);
		o->api = "IB";
		o->orderStatus = OrderStatus::OS_Submitted;
		m_pClient->placeOrder(o->brokerOrderId, contract, oib);

		sendOrderStatus(o->serverOrderId);
	}

	void IBBrokerage::requestNextValidOrderID()
	{
		static int tmp = 1;
		LOG_INFO("Requesting next order id, reqId = {}", tmp);
		if (bkstate_ < BK_GETORDERIDACK)
			bkstate_ = BK_GETORDERIDACK;
		if (m_brokerOrderId == -1)
			m_pClient->reqIds(tmp++);
	}

	//1.from web/distance too long/tws cancel?
	void IBBrokerage::cancelOrder(int oid)
	{
		INFO("Cancel Order {}",(long)oid);
		auto o = OrderManager::instance().retrieveOrderFromServerOrderId(oid);

		if (o != nullptr) {
			m_pClient->cancelOrder(o->brokerOrderId);
		}
		else {
			INFO("Cancel Order id not found. {}",(long)oid);
		}
	}

	// cancel all orders for this symbol
	void IBBrokerage::cancelOrders(const string& symbol)
	{
		auto v = OrderManager::instance().retrieveNonFilledOrderPtr(symbol);

		for (std::shared_ptr<MarketRobot::Order> o : v) {
			m_pClient->cancelOrder(o->brokerOrderId);
			INFO("Cancel Order {}",(long)o->serverOrderId);
		}
	}

	void IBBrokerage::cancelAllOrders() {}

	///https://www.interactivebrokers.com/en/software/api/apiguide/java/updateportfolio.htm
	void IBBrokerage::requestBrokerageAccountInformation(const string& account_)
	{
		//// already subscribed in requestMarketDataAccountInformation
		// LOG_INFO(:[%s,%d][%s]Requesting brokerage account info: %s\n", __FILE__, __LINE__, __FUNCTION__, account_.c_str());
		// m_pClient->reqAccountUpdates(true, account_);  // trigger updateAccountValue()
	}

	void IBBrokerage::requestOpenOrders(const string& account_) {
		LOG_INFO("Requesting open orders.");
		m_pClient->reqAllOpenOrders();
	}

	void IBBrokerage::requestOpenPositions(const string& account_) {
		//// already subscribed in requestMarketDataAccountInformation
		// LOG_INFO(:[%s,%d][%s]Requesting open positions.\n", __FILE__, __LINE__, __FUNCTION__);
		//// m_pClient->reqPositions();              // Requests all positions from all accounts
		// m_pClient->reqAccountUpdates(true, account_); 	// triggers updatePortfolio()
	}

	void IBBrokerage::requestOpenOrders() {
		LOG_INFO("Requesting open orders.");
		m_pClient->reqAllOpenOrders();
	}

	void IBBrokerage::reqAllOpenOrders() {
		LOG_INFO("Requesting all open orders.");
		m_pClient->reqAllOpenOrders();
	}

	void IBBrokerage::reqAutoOpenOrders(bool) {

	}

	void IBBrokerage::modifyOrder_SameT(uint64_t oid, double price, int quantity) {
		auto po = OrderManager::instance().retrieveOrderFromServerOrderId(oid);
		po->orderSize = quantity;
		po->limitPrice = price;

		placeOrder(po);
	}

	/*void IBBrokerage::exerciseOptions(TickerId id, const Contract &contract,
	int exerciseAction, int exerciseQuantity, const std::string &account,
	int override) {}*/

	// End of Brokerage part
	//********************************************************************************************//

	//********************************************************************************************//
	// Market data part
	bool IBBrokerage::connectToMarketDataFeed() {
		mkstate_ = MK_CONNECTED;
		return true;
	}

	void IBBrokerage::disconnectFromMarketDataFeed() {
		mkstate_ = MK_DISCONNECTED;
	}

	bool IBBrokerage::isConnectedToMarketDataFeed() const {
		return (m_pClient->isConnected() && (mkstate_ >= MK_CONNECTED));
	}

	void IBBrokerage::processMarketMessages()
	{
		if (!marketdatafeed::heatbeat(5)) {
			disconnectFromMarketDataFeed();
			return;
		}
		switch (mkstate_) {
		case MK_ACCOUNT:
			if (bkstate_ == BK_READYTOORDER)			// wait for brokerage initialization
				requestMarketDataAccountInformation(CConfig::instance().account);
			break;
		case MK_ACCOUNTACK:
			break;
		case MK_REQCONTRACT:
			requestContractDetails();
			break;
		case MK_REQCONTRACT_ACK:
			break;
			//case MK_REQHISTBAR:
			//  requestHistData();
			//  break;
		case MK_REQREALTIMEDATA:
			if (_mode == TICKBAR) {
				subscribeMarketData();
			}
			else if (_mode == DEPTH) {
				subscribeMarketDepth();
			}
			break;
		case MK_REQREALTIMEDATAACK:
			break;
		case MK_STOP:
			disconnectFromMarketDataFeed();
			break;
		}
	}

	void IBBrokerage::subscribeMarketData() {
		LOG_INFO("Subscribing to market data.");
		// 236 - shortable; 256 - inventory[error(x)]
		//static const char* gt = "100,101,104,106,165,221,225,233,236,258,411";
		static const char* gt =
			"100,101,104,105,106,107,165,221,225,233,236,258,293,294,295,318,411";
		TagValueListSPtr mktDataOptions;

		int i = 0;
		for (auto it = CConfig::instance().securities.begin(); it != CConfig::instance().securities.end(); ++it)
		{
			Contract c;
			SecurityFullNameToContract(*it, c);
			LOG_INFO("subscribe to {}({})",c.localSymbol, c.conId);
			//m_pClient->reqMktData(i, c, gt, false, mktDataOptions); // v976 changed
			m_pClient->reqMktData(i, c, "", false, false,mktDataOptions);
			// whatToShow=TRADES useRTH=false
			m_pClient->reqRealTimeBars(BARREQUESTSTARTINGPOINT + i, c, 5, "TRADES", false, mktDataOptions);
			i++;
		}

		mkstate_ = MK_REQREALTIMEDATAACK;

		//ReqMkDepth();
	}

	void IBBrokerage::unsubscribeMarketData(TickerId reqId) {
		LOG_INFO("Cancel market data {}.",reqId);
		m_pClient->cancelMktData(reqId);
	}

	/*TWS currently limits users to a maximum of 3 distinct market depth requests.
	This same restriction applies to API clients, however API clients may make
	multiple market depth requests for the same security.*/
	void IBBrokerage::subscribeMarketDepth() {
		LOG_INFO("Subscribing to market depth.");

		static const int IBLIMITMKDEPTHNUM = 3;
		TagValueListSPtr mktDataOptions;

		int i = 0;
		for (auto it = CConfig::instance().securities.begin(); it != CConfig::instance().securities.end(); ++it) {
			Contract c;
			SecurityFullNameToContract(*it, c);
			c.exchange = "ISLAND";

			LOG_INFO("Market depth subscribed to contract {}, {}.",c.symbol, c.exchange);
			//m_pClient->reqMktDepth(i + 2000, c, 10, mktDataOptions); v976 changed
			m_pClient->reqMktDepth(i + 2000, c, 10, false, mktDataOptions);

			if (i >= IBLIMITMKDEPTHNUM)
				break;
		}
		mkstate_ = MK_REQREALTIMEDATAACK;
	}

	void IBBrokerage::unsubscribeMarketDepth(TickerId reqId) {
		LOG_INFO("Cancel market depth {}.", reqId);

		m_pClient->cancelMktDepth(reqId,false);
	}

	void IBBrokerage::subscribeRealTimeBars(TickerId id, const Security& security, int barSize, const string& whatToShow, bool useRTH) {}
	void IBBrokerage::unsubscribeRealTimeBars(TickerId tickerId) {}
	void IBBrokerage::requestContractDetails()
	{
		LOG_INFO("Requesting contract details.");

		//::Contract c;
		//SecurityFullNameToContract("SPY_STK_SMART_USD", c);
		//m_pClient->reqContractDetails(4000, c);

		int i = 1;
		for (auto it = CConfig::instance().securities.begin(); it != CConfig::instance().securities.end(); ++it)
		{
			Contract c;
			SecurityFullNameToContract(*it, c);
			m_pClient->reqContractDetails(4000 + i, c);
			i++;
		}

		if (mkstate_ < MK_REQCONTRACT_ACK) {
			mkstate_ = MK_REQCONTRACT_ACK;
		}
	}

	// enddate in "yyyyMMdd HH:mm:ss"
	// duration in seconds
	// barsize in seconds
	// useRTH = "0" or "1"
	std::vector<string> histreqeuests_;				// store symbol
	void IBBrokerage::requestHistoricalData(string fullsymbol, string enddate, string duration, string barsize, string useRTH) {
		int date_format = 1;	// 1: yyyymmdd{space}{space}hh:mm:dd  // 2 long integer specifying the number of seconds since 1/1/1970 GM	
		::Contract contract;
		SecurityFullNameToContract(fullsymbol, contract);

		if (fullsymbol.find("STK") != string::npos)
			contract.includeExpired = false;
		else
			contract.includeExpired = true;

		int useRegularTradingHour = useRTH == "1" ? 1 : 0;

		int interval = std::stoi(barsize);
		string barSize_;
		switch (interval)
		{
		case 1:
			barSize_ = "1 secs";		// not 1 sec
			break;
		case 5:
			barSize_ = "5 secs";
			break;
		case 15:
			barSize_ = "15 secs";
			break;
		case 30:
			barSize_ = "30 secs";
			break;
		case 60:
			barSize_ = "1 min";
			break;
		case 120:
			barSize_ = "2 mins";
			break;
		case 180:
			barSize_ = "3 mins";
			break;
		case 300:
			barSize_ = "5 mins";
			break;
		case 900:
			barSize_ = "15 mins";
			break;
		case 1800:
			barSize_ = "30 mins";
			break;
		case 3600:
			barSize_ = "1 hour";
			break;
		case 86400:
			barSize_ = "1 day";
			break;
		default:
			barSize_ = "1 secs";
			break;
		}

		string durationString_ = duration + " S";			// currently only consider intraday bar, so duration <= 1day
		histreqeuests_.push_back(fullsymbol);
		::TagValueListSPtr charOptions_;
		m_pClient->reqHistoricalData(6000 + histreqeuests_.size() - 1, contract, enddate, durationString_, barSize_,
			"TRADES", useRegularTradingHour, date_format, false, charOptions_);

	}

	// See requestBrokerageAccountInformation()
	void IBBrokerage::requestMarketDataAccountInformation(const string& account) {
		LOG_INFO("Requesting market data account info.");

		m_pClient->reqAccountUpdates(true, account);		// it doesn't trigger updatePortfolio if there is no position

		if (mkstate_ < MK_REQCONTRACT) {		// if mkstate_ < MK_ACCOUNTACK
			mkstate_ = MK_REQCONTRACT;
		}
	}
	// End of Market data
	//********************************************************************************************//

	//********************************************************************************************//
	// events from EWrapper
	// Every tickPrice callback is followed by a tickSize.
	void IBBrokerage::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib) {
		if (field == TickType::LAST)
		{
			lastPriceCache_[tickerId] = price;
		}
		else if (field == TickType::BID)
		{
			bidPriceCache_[tickerId] = price;
		}
		else if (field == TickType::ASK)
		{
			askPriceCache_[tickerId] = price;
		}
		else
		{
			return;
		}
	}

	void IBBrokerage::tickSize(TickerId tickerId, TickType field, int size) {
		Tick k;
		k.fullsymbol_ = CConfig::instance().securities[tickerId];
		k.size_ = size;
		k.time_ = hmsf();
		k.data_time_ = time::now_in_nano();


		if (field == TickType::LAST_SIZE)
		{
			k.datatype_ = DataType::DT_Trade;
			k.price_ = lastPriceCache_[tickerId];
		}
		else if (field == TickType::BID_SIZE)
		{
			k.datatype_ = DataType::DT_Bid;
			k.price_ = bidPriceCache_[tickerId];
		}
		else if (field == TickType::ASK_SIZE)
		{
			k.datatype_ = DataType::DT_Ask;
			k.price_ = askPriceCache_[tickerId];
		}
		else
		{
			return;
		}

		msgq_pub_->sendmsg(k.serialize());

	}

	///https://www.interactivebrokers.com/en/software/api/apiguide/java/orderstatus.htm
	// Note that TWS orders have a fixed clientId and orderId of 0 that distinguishes them from API orders.
	void IBBrokerage::orderStatus(OrderId orderId, const std::string &status, double filled,
		double remaining, double avgFillPrice, int permId, int parentId,
		double lastFillPrice, int clientId, const std::string& whyHeld)
	{
		LOG_INFO("Order status, oid = {}.", orderId);

		if (status == "Cancelled") {
			std::shared_ptr<MarketRobot::Order> o = OrderManager::instance().retrieveOrderFromBrokerOrderIdAndApi(orderId, "IB");

			if (o != nullptr) {
				OrderManager::instance().gotCancel(o->serverOrderId);
				sendOrderStatus(o->serverOrderId);
			}
			else {
				INFO("canceled order not found, oid = {}", o->serverOrderId);
			}
		}
	}

	void IBBrokerage::openOrder(OrderId oid, const Contract& contract,
		const ::Order& order, const OrderState& ostat)
	{
		// Will be called at connection; it seems all open orders have oid = 0
		if (ostat.warningText.empty())
		{
			INFO("Open orders, oid = {}, status={}, orderid={}, permid={}, clientid={%lu}", oid, ostat.status, order.orderId, order.permId, order.clientId);
		}
		else
		{
			INFO("Open orders, oid = {}, status={}, warning={}.", oid, ostat.status, ostat.warningText);
			sendGeneralMessage(ostat.warningText);
		}

		if (ostat.status != "Submitted" && ostat.status != "Filled" && ostat.status != "PreSubmitted")
		{
			// igore other states
			return;
		}
		else
		{
			std::shared_ptr<MarketRobot::Order> o = OrderManager::instance().retrieveOrderFromBrokerOrderIdAndApi(oid, "IB");

			// not found, or found but permId not equal (given permId not default -1) --> existing open order
			if ((o == nullptr) || ((o->permId != order.permId) && (o->permId != -1))) {
				INFO("open order not yet tracked, oid = {}", oid);
				string fullSymbol;
				ContractToSecurityFullName(fullSymbol, contract);

				// create an order
				lock_guard<mutex> g(oid_mtx);
				// assert(m_orderId >= 0);			// start with 0

				auto o2 = make_shared<MarketRobot::Order>();
				o2->account = CConfig::instance().account;
				o2->api = "IB";
				o2->clientOrderId = -1;
				o2->fullSymbol = fullSymbol;
				o2->orderSize = (order.action == "BUY" ? order.totalQuantity : (-order.totalQuantity));            // BUY, SELL, SSHORT
				o2->clientId = order.clientId;
				o2->limitPrice = order.lmtPrice;
				o2->stopPrice = order.auxPrice;
				o2->permId = order.permId;
				o2->orderStatus = OrderStatus::OS_Acknowledged;     // OrderManager::instance().gotOrder
				o2->orderFlag = OrderFlag::OF_OpenPosition;

				o2->serverOrderId = m_serverOrderId;
				o2->brokerOrderId = oid;
				o2->permId = order.permId;
				o2->createTime = ymdhmsf();				// time(nullptr);
				o2->orderType = order.orderType;

				m_serverOrderId++;
				// m_brokerOrderId++;
				if (m_brokerOrderId <= (oid + 1))
					m_brokerOrderId = oid + 1;

				OrderManager::instance().trackOrder(o2);
				sendOrderStatus(o2->serverOrderId);
			}
			else {
				if (o->permId == -1) {
					o->permId = order.permId;
				}

				OrderManager::instance().gotOrder(o->serverOrderId);
				sendOrderStatus(o->serverOrderId);			// acknowledged
			}
		}
	}

	void IBBrokerage::openOrderEnd()
	{
		INFO("Open orders end.");
	}

	void IBBrokerage::updateAccountValue(const std::string& key, const std::string& val,
		const std::string& currency, const std::string& accountName)
	{
		LOG_INFO("Update account value: {},{},{},{}.", key, val, currency, accountName);
		
		if ((currency == "USD") || (currency == ""))
			PortfolioManager::instance()._account.setvalue(key, val, currency);
	}

	// triggered by reqAccountUpdate(true, null) called in Start()
	void IBBrokerage::updatePortfolio(const Contract& contract, double position,
		double marketPrice, double marketValue, double averageCost,
		double unrealizedPNL, double realizedPNL, const std::string& accountName)
	{
		LOG_INFO("Update portfolio: {},{}, {:.3f}", contract.localSymbol, position, averageCost);

		//Note: after closing position, position is 0 
		//Don't send message when position is 0
		string symbol;
		ContractToSecurityFullName(symbol, contract);
		if (position != 0) {
			Position pos;
			pos._fullsymbol = symbol;
			pos._size = position;
			pos._avgprice = averageCost;
			pos._openpl = unrealizedPNL;
			pos._closedpl = realizedPNL;
			pos._account = CConfig::instance().account;
			pos._api = "IB";
			PortfolioManager::instance().Add(pos);
			sendOpenPositionMessage(pos);
		}

		if (mkstate_ < MK_REQCONTRACT) {
			mkstate_ = MK_REQCONTRACT;
		}
	}

	void IBBrokerage::updateAccountTime(const std::string& timeStamp)
	{
		LOG_INFO("Update Account Time: {}", timeStamp);
		
		// Trigger Account Message; once account has been updated.
		sendAccountMessage();
	}

	void IBBrokerage::nextValidId(::OrderId orderid)
	{
		//if (orderid >= m_orderId) {		// it seems ib is connected twice
		if (orderid > m_brokerOrderId) {
			INFO("client id={} next_valid_order_id = {}", m_pClient->clientId(), orderid);
			m_brokerOrderId = orderid;
			bkstate_ = BK_READYTOORDER;
		}
	}

	void IBBrokerage::contractDetails(int reqId, const ContractDetails& contractDetails)
	{
		if (mkstate_ < MK_REQREALTIMEDATA) {
			mkstate_ = MK_REQREALTIMEDATA;
		}
		LOG_INFO("reqid={}, symbol={}", reqId, contractDetails.longName);
		string symbol;		// full symbol
		ContractToSecurityFullName(symbol, contractDetails.contract);

		auto it = DataManager::instance().securityDetails_.find(symbol);
		if (it == DataManager::instance().securityDetails_.end()) {
			Security s;
			s.symbol = contractDetails.contract.localSymbol;
			s.exchange = contractDetails.contract.exchange;
			s.securityType = contractDetails.contract.secType;
			s.multiplier = contractDetails.contract.multiplier;
			s.localName = contractDetails.longName;
			s.ticksize = std::to_string(contractDetails.minTick);

			DataManager::instance().securityDetails_[symbol] = s;
		}

		sendContractMessage(symbol, contractDetails.longName, std::to_string(contractDetails.minTick));
	}

	void IBBrokerage::contractDetailsEnd(int reqId)
	{
		LOG_INFO("Contract details end. reqid={}", reqId);
	}

	void IBBrokerage::execDetails(int reqId, const Contract& contract, const Execution& execution)
	{
		LOG_INFO("Execution (Fill) details. reqid={}.",reqId);

		Fill t;
		ContractToSecurityFullName(t.fullSymbol, contract);
		t.tradetime = ymdhmsf();
		t.brokerOrderId = execution.orderId;
		t.tradeId = execution.permId;			// std::stoi(execution.execId);
		t.tradePrice = execution.price;
		t.tradeSize = (execution.side == "BOT" ? 1 : -1)*execution.shares;

		auto o = OrderManager::instance().retrieveOrderFromBrokerOrderIdAndApi(execution.orderId, "IB");

		if (o != nullptr) {
			t.serverOrderId = o->serverOrderId;
			t.clientOrderId = o->clientOrderId;
			t.brokerOrderId = o->brokerOrderId;
			t.account = o->account;
			t.api = o->api;

			OrderManager::instance().gotFill(t);
			// sendOrderStatus(o->serverOrderId);
			sendOrderFilled(t);		// BOT SLD
		}
		else {
			INFO("Fill cant find matching order; brokerage id = {}",execution.orderId);

			t.serverOrderId = -1;
			t.clientOrderId = -1;
			t.account = CConfig::instance().account;
			t.api = "IB";

			sendOrderFilled(t);		// BOT SLD
		}
	}

	//Error Code: https://www.interactivebrokers.com/en/software/api/apiguide/tables/api_message_codes.htm
	void IBBrokerage::error(const int id, const int errorCode, const std::string errorString) {
		LOG_ERROR("id={},eCode={},msg:{}.", id, errorCode, errorString);
		sendGeneralMessage(to_string(id) + SERIALIZATION_SEPARATOR + to_string(errorCode) + SERIALIZATION_SEPARATOR + errorString);

		/*if (errorCode == 202)			// order cancelled, moved to order status callback
		{
		OrderManager::instance().gotCancel(id);
		sendOrderCancelled(id);
		}*/
		if (id == -1 && errorCode == 1100) { // if "Connectivity between IB and TWS has been lost"
			disconnectFromBrokerage();
			disconnectFromMarketDataFeed();
		}
		else if (errorCode == 326) {
			LOG_ERROR("ClientId duplicated! bump up clientID and reconnect!!. Error={}", errorString);
			disconnectFromBrokerage();
			disconnectFromMarketDataFeed();
			connectToBrokerage();		// reconnect.
			connectToMarketDataFeed();
			//exit(0);
		}
	}

	// postion = depth
	void IBBrokerage::updateMktDepth(TickerId id, int position, int operation, int side, double price, int size) {
		const char* sidestr = (side == 1) ? "BID_PRICE" : "ASK_PRICE";	 // side 0 for ask, 1 for bid
		INFO("Update market depth: {} {} {} {} {:.3f} {%d}",
			CConfig::instance().securities[id - 1000], operation, sidestr, position, price, size);
	}

	// postion = depth
	void IBBrokerage::updateMktDepthL2(TickerId id, int position, std::string marketMaker, int operation,
		int side, double price, int size) {
		LOG_INFO("Update market depth L2.");
		const char* sidestr = (side == 1) ? "BID_PRICE" : "ASK_PRICE";	 // side 0 for ask, 1 for bid
		cout << CConfig::instance().securities[id - 1000] << " " \
			<< operation << " " << sidestr << " " \
			<< position << " " << price << " " << size << "\n";
	}

	// triggered by EClientSocket::reqManagedAccts
	// feed in upon connection
	void IBBrokerage::managedAccounts(const std::string& accountsList) {
		LOG_INFO("client_id={},the managed account is:{}.", m_pClient->clientId(), accountsList);

		if (CConfig::instance().account != accountsList) {
			ERROR("Config account {} does not match IB account {}!",
				CConfig::instance().account, accountsList);
			disconnectFromBrokerage();
			gShutdown = true;
			//exit(1);
		}
	}

	// Intra-day bar sizes are relayed back in Local Time Zone, daily bar sizes and greater are relayed back in Exchange Time Zone.
	void IBBrokerage::historicalData(TickerId reqId, const std::string& date, double open, double high,
		double low, double close, int volume, int barCount, double WAP, int hasGaps) {
		try {
			string symbol = histreqeuests_[reqId];
			sendHistoricalBarMessage(symbol, date, open, high, low, close, volume, barCount, WAP);
		}
		catch (...) {
			sendGeneralMessage("Historical Data Error");
		}
	}

	void IBBrokerage::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
		long volume, double wap, int count) {
		string symbol;
		uint64_t index = reqId - BARREQUESTSTARTINGPOINT;
		if (reqId >= BARREQUESTSTARTINGPOINT) {
			symbol = CConfig::instance().securities[index];
		}
		else {
			return;
		}
		Bar* b = new Bar(symbol, 5, open, high, low, close, volume,count);
		b->start_time_ = time * time_unit::NANOSECONDS_PER_SECOND;
		//DEBUG("{}", b->serialize());
		MR::DC::DataCenter::instance().push_bar(b);
		/*string time_str = timestamp_readble(time);
		LOG_INFO("{}|{}|{:.2f}|{:.2f}|{:.2f}|{:.2f}|{:4d}|{:4d}|",
			symbol, time_str ,open, high, low, close, volume, count);*/
	}

	//! [connectack]
	void IBBrokerage::connectAck() {
		if (!m_extraAuth && m_pClient->asyncEConnect())
			m_pClient->startApi();
	}
	//! [connectack]

	// end of events from EWrapper
	//********************************************************************************************//

	//********************************************************************************************//
	// auxilliary functions
	//********************************************************************************************//
	// symbol sectype exchange right multiplier
	void IBBrokerage::SecurityFullNameToContract(const std::string& symbol, Contract& c)
	{
		vector<string> v = stringsplit(symbol, '_');
		DEBUG("Contract:{}", symbol);
		c.localSymbol = v[0];
		c.secType = v[1];
		c.exchange = v[2];
		c.currency = v[3];

		if (v.size() == 5)
		{
			c.multiplier = v[4];
		}
	}

	void IBBrokerage::ContractToSecurityFullName(std::string& symbol, const Contract& c)
	{
		char sym[128] = {};
		if ((c.multiplier == "1") || (c.multiplier == ""))
			if (c.secType == "CASH")
				sprintf(sym, "%s_%s_%s", c.localSymbol.c_str(), c.secType.c_str(), c.primaryExchange.c_str());
			else
				sprintf(sym, "%s_%s_SMART", c.localSymbol.c_str(), c.secType.c_str());
		else
			if (c.exchange == "")
				sprintf(sym, "%s_%s_%s_%s", c.localSymbol.c_str(), c.secType.c_str(), c.primaryExchange.c_str(), c.multiplier.c_str());
			else
				sprintf(sym, "%s_%s_%s_%s", c.localSymbol.c_str(), c.secType.c_str(), c.exchange.c_str(), c.multiplier.c_str());

		symbol = sym;
	}

	void IBBrokerage::OrderToIBOfficialOrder(std::shared_ptr<MarketRobot::Order> o, ::Order& oib)
	{
		oib.auxPrice = o->trailPrice != 0 ? (double)o->trailPrice : (double)o->stopPrice;
		oib.lmtPrice = (double)o->limitPrice;

		// Only MKT, LMT, STP, and STP LMT, TRAIL, TRAIL LIMIT are supported
		oib.orderType = o->orderType;

		oib.totalQuantity = std::abs(o->orderSize);
		oib.action = o->orderSize > 0 ? "BUY" : "SELL";         // SSHORT not supported here

		oib.tif = o->timeInForce;
		oib.outsideRth = true;
		//order.OrderId = (int)o.id;
		oib.transmit = true;

		oib.account = o->account.empty() ? CConfig::instance().account : o->account;
		// Set up IB order Id
		oib.orderId = o->brokerOrderId;
	}
	// end of auxilliary functions
	//********************************************************************************************//
}