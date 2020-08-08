async [Transaction] txn_new_order(const uint32_t& warehouse_id ) {
  const int32_t g_new_order_remote_item_pct = 50;
  bool g_new_order_fast_id_gen = true;

  int r = 1;
  const uint32_t districtID = RandomNumber(r, 1, 10);
  const uint32_t customerID = GetCustomerId(r);
  const uint32_t numItems = RandomNumber(r, 5, 15);
  uint32_t itemIDs[15], supplierWarehouseIDs[15], orderQuantities[15];
    
  for (uint32_t i = 0; i < numItems; i++) {
    itemIDs[i] = GetItemId(r);
    supplierWarehouseIDs[i] = warehouse_id;
    
    if ( NWAREHOUSE == (uint64_t)1 || RandomNumber(r, 1, 100) > g_new_order_remote_item_pct ) {
      supplierWarehouseIDs[i] = warehouse_id;
    } else {
      do {
        supplierWarehouseIDs[i] = RandomNumber(r, 1, NWAREHOUSE);
      } while (supplierWarehouseIDs[i] == warehouse_id);
    }
    
    orderQuantities[i] = RandomNumber(r, 1, 10);
  }
  
  customer_key c_key;
  c_key.c_w_id = warehouse_id;
  c_key.c_d_id = districtID;
  c_key.c_id = customerID;


  customer_value c_value = getCustomerValueInWarehouses(c_key);
    
  warehouse_key w_key;
  w_key.w_id = warehouse_id;
  warehouse_value w_value = getWarehouseValueInWarehouses(w_key);
    
  district_key d_key;
  d_key.d_w_id = warehouse_id;
  d_key.d_id = districtID;
  district_value d_value = getDistrictValueInWarehouses(d_key);
    
  const uint64_t my_next_o_id = g_new_order_fast_id_gen ?
    FastNewOrderIdGen(r, warehouse_id, districtID) : d_value.d_next_o_id;

  new_order_key no_key;
  no_key.no_w_id = warehouse_id;
  no_key.no_d_id = districtID;
  no_key.no_o_id = my_next_o_id;
  new_order_value no_value;
  putNewOrderInWarehouses(no_key, no_value);
    
  oorder_key oo_key;
  oo_key.o_w_id = warehouse_id;
  oo_key.o_d_id = districtID;
  oo_key.o_id =  no_key.no_o_id;
  oorder_value oo_value;
  oo_value.o_c_id = int32_t(customerID);
  oo_value.o_carrier_id = 0; // seems to be ignored
  oo_value.o_ol_cnt = int8_t(numItems);
  oo_value.o_all_local = false;
  oo_value.o_entry_d = GetCurrentTimeMillis();
  putOorderInWarehouses(oo_key, oo_value);
    
  oorder_c_id_idx_key oo_idx_key;
  oo_idx_key.o_w_id = warehouse_id;
  oo_idx_key.o_d_id = districtID;
  oo_idx_key.o_c_id = customerID;
  oo_idx_key.o_o_id = no_key.no_o_id;
  oorder_c_id_idx_value oo_idx_value;
  oo_idx_value.o_dummy = 0;
  putOorderIdxInWarehouses(oo_idx_key, oo_idx_value);


  for (uint32_t ol_number = 1; ol_number <= numItems; ol_number++) {
    const uint32_t ol_supply_w_id = supplierWarehouseIDs[ol_number - 1];
    const uint32_t ol_i_id = itemIDs[ol_number - 1];
    const uint32_t ol_quantity = orderQuantities[ol_number - 1];

    item_key i_key;
    i_key.i_id = ol_i_id;
    item_value i_value = getItemValueInItems(i_key);
      
    stock_key s_key;
    s_key.s_w_id = ol_supply_w_id;
    s_key.s_i_id = ol_i_id;
    stock_value s_value = getStockValueInWarehouses(s_key);
      
    stock_value s_new_value;
    s_new_value.s_quantity = s_value.s_quantity;
    s_new_value.s_ytd = s_value.s_ytd;
    s_new_value.s_order_cnt = s_value.s_order_cnt;
    s_new_value.s_remote_cnt = s_value.s_order_cnt;

    if ( s_new_value.s_quantity - ol_quantity >= 10)
      s_new_value.s_quantity -= ol_quantity;
    else
      s_new_value.s_quantity += -int32_t(ol_quantity) + 91;
    
    s_new_value.s_ytd += ol_quantity;
    s_new_value.s_remote_cnt += (ol_supply_w_id == warehouse_id) ? 0 : 1;

    putStockInWarehouses(s_key, s_new_value);
      
    order_line_key ol_key;
    ol_key.ol_w_id = warehouse_id;
    ol_key.ol_d_id = districtID;
    ol_key.ol_o_id = no_key.no_o_id;
    ol_key.ol_number = ol_number;

    order_line_value ol_value;
    ol_value.ol_i_id = int32_t(ol_i_id);
    ol_value.ol_delivery_d = 0; // not delivered yet
    ol_value.ol_amount = float(ol_quantity) * i_value.i_price;
    ol_value.ol_supply_w_id = int32_t(ol_supply_w_id);
    ol_value.ol_quantity = int8_t(ol_quantity);

    putOrderLineInWarehouses(ol_key, ol_value);
  }
}

async [Warehouses] txn_payment(const uint32_t& warehouse_id) {
  int r = 1;
  int cwId = (int) warehouse_id / WAREHOUSE_MAX_BATCH_SIZE;
 
  const uint32_t districtID = RandomNumber(r, 1, NumDistrictsPerWarehouse);
  uint32_t customerDistrictID, customerWarehouseID;
  if ( NWAREHOUSE == 1 || RandomNumber(r, 1, 100) <= 85) {
    customerDistrictID = districtID;
    customerWarehouseID = warehouse_id;
  } else {
    customerDistrictID = RandomNumber(r, 1, NumDistrictsPerWarehouse);
    do {
      customerWarehouseID = RandomNumber(r, 1, NWAREHOUSE);
    } while (customerWarehouseID == warehouse_id);
  }
  const float paymentAmount = (float) (RandomNumber(r, 100, 500000) / 100.0);
  const uint32_t ts = GetCurrentTimeMillis();
  
  warehouse_key k_w;
  k_w.w_id = warehouse_id;

  warehouse_value v_w;
  if( warehouseIds.count(cwId) == 0 ) {
    v_w.valid = false;
  } else {
    v_w = getWarehouseValueFromTable(cwId, k_w);
  }
  warehouse_value v_w_new;

  initWarehouseValue(v_w_new, v_w);
  

  v_w_new.w_ytd += paymentAmount;
  if( warehouseIds.count(cwId) == 0) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", cwId);
    mace::string c = getContextName(" WarehouseTable", cwId);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(w);
  }
  putWarehouseToTable(cwId, k_w, v_w_new);
 
  district_key k_d;
  k_d.d_w_id = warehouse_id;
  k_d.d_id = districtID;
  int cdId = (int)k_d.d_id / DISTRICT_MAX_BATCH_SIZE;
  district_value v_d = getDistrictValueInWarehouse(cwId, cdId, k_d);
    
  district_value v_d_new;
  initDistrictValue(v_d_new, v_d);
  
  v_d_new.d_ytd += paymentAmount;
  putDistrictInWarehouse(cwId, k_d, v_d_new);
    

  customer_key k_c;
  customer_value v_c;
  if (RandomNumber(r, 1, 100) <= 60) {
    // cust by name
    char lastname_buf[CustomerLastNameMaxSize + 1];
    //static_assert(sizeof(lastname_buf) == 16, "xx");
    GetNonUniformCustomerLastNameRun(lastname_buf, r);

    static const std::string zeros(16, 0);
    static const std::string ones(16, 255);

    customer_name_idx_key k_c_idx_0;
    k_c_idx_0.c_w_id = customerWarehouseID;
    k_c_idx_0.c_d_id = customerDistrictID;
    k_c_idx_0.c_last.assign((const char *) lastname_buf, 16);
    k_c_idx_0.c_first.assign(zeros);

    customer_name_idx_key k_c_idx_1;
    k_c_idx_1.c_w_id = customerWarehouseID;
    k_c_idx_1.c_d_id = customerDistrictID;
    k_c_idx_1.c_last.assign((const char *) lastname_buf, 16);
    k_c_idx_1.c_first.assign(ones);

    const int customer_w = (int) customerWarehouseID / WAREHOUSE_MAX_BATCH_SIZE;
    if( warehouseIds.count(customer_w) ) {

      mace::map<customer_name_idx_key, customer_name_idx_value> c = scanCustomerNameIdxInWarehouse(k_c_idx_0, k_c_idx_1);
      ASSERT(c.size() > 0);
      int index = c.size() / 2;
      if (c.size() % 2 == 0)
        index--;
    
      mace::map<customer_name_idx_key, customer_name_idx_value>::iterator iter = c.begin();
      for(int i=0; i< index; i++) iter++;
      customer_name_idx_value v_c_idx;
      v_c_idx.c_id = (iter->second).c_id; 

      k_c.c_w_id = customerWarehouseID;
      k_c.c_d_id = customerDistrictID;
      k_c.c_id = v_c_idx.c_id;
      v_c = getCustomerValueInWarehouse(customer_w, k_c);
    }
  } else {
     // cust by ID
      const uint32_t customerID = GetCustomerId(r);
      k_c.c_w_id = customerWarehouseID;
      k_c.c_d_id = customerDistrictID;
      k_c.c_id = customerID;

      cwId = (int) k_c.c_w_id / WAREHOUSE_MAX_BATCH_SIZE;
      v_c = getCustomerValueInWarehouse(cwId, k_c);
  }
    
  customer_value v_c_new;
  initCustomerValue(v_c_new, v_c);

  v_c_new.c_balance -= paymentAmount;
  v_c_new.c_ytd_payment += paymentAmount;
  v_c_new.c_payment_cnt++;
  cwId = (int) k_c.c_w_id / WAREHOUSE_MAX_BATCH_SIZE;
  if( warehouseIds.count(cwId) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", cwId);
    mace::string c = getContextName(" WarehouseTable", cwId);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(cwId);
  }
  putCustomerInWarehouse(cwId, k_c, v_c_new);
  
  history_key k_h;
  k_h.h_c_id = k_c.c_id;
  k_h.h_c_d_id = k_c.c_d_id;
  k_h.h_c_w_id = k_c.c_w_id;
  k_h.h_d_id = districtID;
  k_h.h_w_id = warehouse_id;
  k_h.h_date = ts;
  history_value v_h;
  v_h.h_amount = paymentAmount;
  char tbuf[500];
  snprintf(tbuf, 500 + 1, "%.10s    %.10s", v_w.w_name.c_str(), v_d.d_name.c_str());
  v_h.h_data = tbuf;

  cwId = (int) k_h.h_c_w_id / WAREHOUSE_MAX_BATCH_SIZE;
  if( warehouseIds.count(cwId) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", cwId);
    mace::string c = getContextName(" WarehouseTable", cwId);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(cwId);
  }
  putHistoryInWarehouse(cwId, k_h, v_h);   
}


async [Warehouse<cwId>] void txn_delivery(const int& cwId, const uint32_t& warehouse_id) {
  int r = 1;
  const uint32_t o_carrier_id = RandomNumber(r, 1, NumDistrictsPerWarehouse);
  const uint32_t ts = GetCurrentTimeMillis();

  for (uint32_t d = 1; d <= NumDistrictsPerWarehouse; d++) {
    const int cdId = (int) d/ DISTRICT_MAX_BATCH_SIZE;
    new_order_key k_no_0;
    k_no_0.no_w_id = warehouse_id;
    k_no_0.no_d_id = d;
    k_no_0.no_o_id = last_no_o_ids[d - 1];
    new_order_key k_no_1;
    k_no_1.no_w_id = warehouse_id;
    k_no_1.no_d_id = d;
    k_no_1.no_o_id = max();
    
    if( districtIds.count(cdId) == 0 ) {
      continue;
    }
    mace::map<new_order_key, new_order_value> new_order_c = scanNewOrderInDistrict(cwId, cdId, k_no_0, k_no_1);
    

    if( new_order_c.empty() ) {
      continue;
    }
    new_order_key k_no = (new_order_c.begin())->first;
      
    //last_no_o_ids[d - 1] = k_no.no_o_id + 1; // XXX: update last seen

    oorder_key k_oo;
    initOorderKey(k_oo, warehouse_id, d, k_no.no_o_id);

    oorder_value v_oo = getOorderValueInDistrict(cwId, cdId, k_oo);
    
    order_line_key k_oo_0;
    initOrderLineKey(k_oo_0, warehouse_id, d, k_no.no_o_id, 0);
    order_line_key k_oo_1;
    initOrderLineKey(k_oo_1, warehouse_id, d, k_no.no_o_id, max() );

    mace::map<order_line_key, order_line_value> c = scanOrderLineInDistrict(cwId, cdId, k_oo_0, k_oo_1);
    float sum = 0.0;
    for (mace::map<order_line_key, order_line_value>::iterator iter = c.begin(); iter!=c.end(); iter++) {
      order_line_value v_ol;
      initOrderLineValue(v_ol, iter->second);
        
      sum += v_ol.ol_amount;
      order_line_value v_ol_new;
      initOrderLineValue(v_ol_new, v_ol);
      v_ol_new.ol_delivery_d = ts;
      putOrderLineInDistrict(cwId, cdId, iter->first, v_ol_new);
        
    }

      // delete new order
    removeNewOrderInDistrict(cwId, cdId, k_no);
    
    oorder_value v_oo_new;
    initOorderValue(v_oo_new, v_oo);
    v_oo_new.o_carrier_id = o_carrier_id;
    putOorderInDistrict(cwId, cdId, k_oo, v_oo_new);
    

    const uint32_t c_id = v_oo.o_c_id;
    const float ol_total = sum;
    customer_key k_c;
    initCustomerKey(k_c, warehouse_id, d, c_id);
    customer_value v_c = getCustomerValueInDistrict(cwId, cdId, k_c);  
    
    customer_value v_c_new;
    initCustomerValue(v_c_new, v_c);
    v_c_new.c_balance += ol_total;
    putCustomerInDistrict(cwId, cdId, k_c, v_c_new);
  }
      
}

[Warehouse<cwId>] void txn_order_status(const uint32_t& cwId, const uint32_t& warehouse_id) {
  int r = 1;
  const uint32_t districtID = RandomNumber(r, 1, NumDistrictsPerWarehouse);
  const int cdId = districtID / MAX_BATCH_SIZE;

  customer_key k_c;
  customer_value v_c;
  if (RandomNumber(r, 1, 100) <= 60) {
      // cust by name
    char lastname_buf[CustomerLastNameMaxSize + 1];
    GetNonUniformCustomerLastNameRun(lastname_buf, r);

    static const std::string zeros(16, 0);
    static const std::string ones(16, 255);

    customer_name_idx_key k_c_idx_0;
    k_c_idx_0.c_w_id = warehouse_id;
    k_c_idx_0.c_d_id = districtID;
    k_c_idx_0.c_last.assign((const char *) lastname_buf, 16);
    k_c_idx_0.c_first.assign(zeros);

    customer_name_idx_key k_c_idx_1;
    k_c_idx_1.c_w_id = warehouse_id;
    k_c_idx_1.c_d_id = districtID;
    k_c_idx_1.c_last.assign((const char *) lastname_buf, 16);
    k_c_idx_1.c_first.assign(ones);

    if( customerNameIdxIds.count(ccId) ) {

      mace::map<customer_name_idx_key, customer_name_idx_value> c = scanCustomerNameIdxInTable(cwId, ccId, k_c_idx_0, k_c_idx_1);
      ASSERT(c.size() > 0);
    
      int index = c.size() / 2;
      if (c.size() % 2 == 0)
        index--;
    
      customer_name_idx_value v_c_idx;
      mace::map<customer_name_idx_key, customer_name_idx_value>::iterator iter = c.begin();
      for(int i=0; i< index; i++) iter++;
    
      initCustomerNameIdxValue(v_c_idx, iter->second);

      k_c.c_w_id = warehouse_id;
      k_c.c_d_id = districtID;
      k_c.c_id = v_c_idx.c_id;

      if( districtIds.count(cdId) ) {
        v_c = getCustomerValueInDistrict(cwId, cdId, k_c);
      }
    }

  } else {
      // cust by ID
    const uint32_t customerID = GetCustomerId(r);
    k_c.c_w_id = warehouse_id;
    k_c.c_d_id = districtID;
    k_c.c_id = customerID;
    if( districtIds.count(cdId) ) {
      v_c = getCustomerValueInDistrict(cwId, cdId, k_c);
    }
 
  }
  bool g_order_status_scan_hack = true;
  oorder_c_id_idx_key newest_o_c_id;
  if (g_order_status_scan_hack) {
      oorder_c_id_idx_key k_oo_idx_0;
      initOorderCIdIdxKey(k_oo_idx_0, warehouse_id, districtID, k_c.c_id, 0);
      oorder_c_id_idx_key k_oo_idx_1;
      initOorderCIdIdxKey(k_oo_idx_1, warehouse_id, districtID, k_c.c_id, max());

      if( districtIds.count(cdId) ) { 
        mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value> c_oorder = scanOorderCIdIdxInDistrict(cwId, cdId, k_oo_idx_0, k_oo_idx_1);
      
        ASSERT(c_oorder.size());
        mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value>::iterator iter = c_oorder.begin();
        initOorderCIdIdxKey(newest_o_c_id, iter->first);
      }
  } else {
    /*
      oorder_c_id_idx_key k_oo_idx_hi;
      initOorderCIdIdxKey( k_oo_idx_hi, warehouse_id, districtID, k_c.c_id, max() );
      
      mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value> c_oorder = rscanOorderCIdIdx(k_oo_idx_hi);
      ASSERT(c_oorder.size() == 1);
      mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value>::iterator iter = c_oorder.begin();
      initOorderCIdIdxKey(newest_o_c_id, iter->first);
      */
  }

  oorder_c_id_idx_key k_oo_idx;
  initOorderCIdIdxKey(k_oo_idx, newest_o_c_id);
  const uint32_t o_id = k_oo_idx.o_o_id;

  order_line_key k_ol_0;
  initOrderLineKey(k_ol_0, warehouse_id, districtID, o_id, 0);
  order_line_key k_ol_1;
  initOrderLineKey(k_ol_1, warehouse_id, districtID, o_id, max());
  if( districtIds.count(cdId) ) {
    mace::map<order_line_key, order_line_value> c_order_line = scanOrderLineInDistrict(cwId, cdId, k_ol_0, k_ol_1);
  }
}

[Warehouse<cwId>] void txn_stock_level(const int& cwId, const uint32_t& warehouse_id) {
  int r = 1;
  const uint32_t districtID = RandomNumber(r, 1, NumDistrictsPerWarehouse);
  const int cdId = (int) districtID / DISTRICT_MAX_BATCH_SIZE;
  const uint32_t threshold = RandomNumber(r, 10, 20);
  
  district_key k_d;
  initDistrictKey(k_d, warehouse_id, districtID);
  if( districtIds.count(cdId) == 0 ) {
    return;
  }
  district_value v_d = getDistrictValueFromTable(cwId, cdId, k_d);
  
  const uint64_t cur_next_o_id = g_new_order_fast_id_gen ?
      NewOrderIdHolder(warehouse_id, districtID) : v_d.d_next_o_id;

  const int32_t lower = cur_next_o_id >= 20 ? (cur_next_o_id - 20) : 0;
  order_line_key k_ol_0;
  initOrderLineKey(k_ol_0, warehouse_id, districtID, lower, 0);
  order_line_key k_ol_1;
  initOrderLineKey(k_ol_1, warehouse_id, districtID, cur_next_o_id, 0);
  
  mace::map<order_line_key, order_line_value> c = scanOrderLineInDistrict(cwId, cdId, k_ol_0, k_ol_1);

    
  std::map<uint32_t, bool> s_i_ids_distinct;
  for (mace::map<order_line_key, order_line_value>::iterator iter = c.begin(); iter != c.end(); iter++) {
    const size_t nbytesread = serializer<int16_t, true>::max_nbytes();

    stock_key k_s;
    initStockKey(k_s, warehouse_id, (iter->second).ol_i_id);

    int csId = (int) (iter->second).ol_i_id / MAX_BATCH_SIZE;
    if( stockIds.count(csId) == 0) {
      continue;
    }
    order_line_value v_ol = getStockValueFromTable(k_s);    
    const uint8_t *ptr = (const uint8_t *) obj_v.data();
    nt16_t i16tmp;
    ptr = serializer<int16_t, true>::read(ptr, &i16tmp);
    if (i16tmp < int(threshold)) {
      s_i_ids_distinct[p.first] = 1;
    }
    
  }
  
    
}