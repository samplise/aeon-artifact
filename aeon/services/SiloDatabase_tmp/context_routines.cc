// Warehouse table

[Warehouses] warehouse_value getWarehouseValueInWarehouses( warehouse_key const& key ) {
  int cwId =(int) key.w_id / WAREHOUSE_MAX_BATCH_SIZE;
  if( warehouseIds.count(cwId) == 0 ) {
    warehouse_value v; 
    v.valid = false;
    return v;
  }

  return getWarehouseValueFromTable(cwId, key); 
}

[WarehouseTable<x>] warehouse_value getWarehouseValueFromTable(const int& x, warehouse_key const& key) {
  mace::map<warehouse_key, warehouse_value>::const_iterator iter = table.find(key);
  if( iter == table.end() ) {
    warehouse_value value;
    value.valid = false;
    return value;
  } else {
    return iter->second;
  }
}

[WarehouseTable<x>] void putWarehouseToTable(const int& x, warehouse_key const& key, warehouse_value const& value) {
  table[key] = value;
}


// New_order
[Warehouses] void putNewOrderInWarehouses(new_order_key const& key, new_order_value const& value) {
  int cwId = (int) key.no_w_id / WAREHOUSE_MAX_BATCH_SIZE;
  
  if( WarehouseIds.count(cwId) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", cwId);
    mace::string c = getContextName(" WarehouseTable", cwId);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(w);
  }

  putNewOrderInWarehouse(cwId, key, value);

}

[Warehouse<w>] void putNewOrderInWarehouse(const int& w, new_order_key const& key, new_order_value const& value) {
  int cdId = (int) key.no_d_id / DISTRICT_MAX_BATCH_SIZE; 

  if( districtIds.count(cdId) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = getContextName("Warehouse", w);
    mace::string m = getContextName("District", w, cdId);
    mace::string c = getContextName("DistrictTable", w, cdId);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );
    
    addNewOwnerships(newOwnerships);

    districtIds.insert(cdId);
  }

  putNewOrderInDistrict(w, cdId, key, value);

}

[District<w, d>] void putNewOrderInDistrict(const int& w, const int& d, new_order_key const& key, new_order_value const& value) {
  int x = (int)key.no_o_id / MAX_BATCH_SIZE;

  if( newOrderIds.count(x) == 0 ) {
    mace::string p = getContextName("District", w, d);
    mace::string c = getContextName("NewOrderTable", w, d, x);

    addNewOwnership(p, c);

    newOrderIds.insert(x);
  }

  putNewOrderToTable(w, d, x, key, value);

}

[NewOrderTable<w, d, x>] void putNewOrderTable(const int& w, const int& d, const int& x, new_order_key const& key, new_order_value const& value) {
  table[key] = value;
}

[District<w ,d>] mace::map<new_order_key, new_order_value> scanNewOrderInDistrict(const int& w, const int& d, const new_order_key& low, const new_order_key& high) {
  ASSERT(low.no_w_id == high.no_w_id && low.no_d_id == high.no_d_id);
  uint32_t w = low.no_w_id;
  uint32_t d = low.no_d_id;

  uint32_t lo = low.no_o_id;
  uint32_t ho = high.no_o_id;

  ASSERT(lo <= ho);
  
  int lx = (int) lo / MAX_BATCH_SIZE;
  int hx = (int) ho / MAX_BATCH_SIZE;

  mace::map<new_order_key, new_order_value> scan_pairs;
  for(int i=lx; i<=hx; i++) {
    if(newOrderIds.count(i) == 0) {
      continue;
    }
    mace::map<new_order_key, new_order_value> s = scanNewOrderInTable(w, d, i, lo, ho);

    mace::map<new_order_key, new_order_value>::const_iterator iter = s.begin();
    for(; iter!=s.end(); iter++) {
      scan_pairs[iter->first] = iter->second;
    }
  }

  return scan_pairs;
}

[NewOrderTable<w, d, x>] mace::map<new_order_key, new_order_value> scanNewOrderInTable(const int& w, const int& d, const int& x, const int& lo, const int& ho) {
  mace::map<new_order_key, new_order_value> scan_pairs;

  mace::map<new_order_key, new_order_value>::const_iterator iter = table.begin();
  for(; iter!=table.end(); iter++) {
    const new_order_key& k = iter->first;
    if( k.no_o_id >= lo && k.no_o_id <= ho){
      scan_pairs[iter->first] = iter->second;
    }
  }
  return scan_pairs;
}

[District<wId, dId>] void removeNewOrderInDistrict(const int& wId, const int& dId, const new_order_key& key) {
  int x = (int)key.no_o_id / MAX_BATCH_SIZE;
  if( newOrderIds.count(x) == 0) {
    return;
  }

  removeNewOrderFromTable(w, d, x, key);
}

[NewOrderTable<w, d, x>] void removeNewOrderFromTable(const int w, const int d, const int x, const new_order_key& key) {
  table.erase(key);
}

// District table
[Warehouses] district_value getDistrictValueInWarehouses( district_key const& key ) {
  int w = key.d_w_id;
  int cwId = (int) w/WAREHOUSE_MAX_BATCH_SIZE;

  if( warehouseIds.count(cwId) == 0 ) {
    customer_value value;
    value.valid = false;
    return value;
  }

  return getDistrictValueInWarehouse( cwId, key )
}

[Warehouse<wId>] district_value getDistrictValueInWarehouse( const int32_t& wId, district_key const& key ) {
  int w = key.d_w_id;
  int cwId = (int) w/WAREHOUSE_MAX_BATCH_SIZE;
  ASSER( cwId == wId );

  int d = key.d_id;
  int cdId = (int) d/ DISTRICT_MAX_BATCH_SIZE;

  if( districtIds.count(cdId) == 0 ) {
    district_value value;
    value.valid = false;
    return value;
  }

  return getDistrictValueFromTable( cwId, cdId, key )
}

[DistrictTable<wId, dId>] district_value getDistrictValueFromTable(const int& wId, const int& dId, district_key const& key) {
  mace::map<district_key, district_value>::const_iterator iter = table.find(key);
  if( iter == table.end() ) {
    district_value value;
    value.valid = false;
    return value;
  } else {
    return iter->second;
  }
}

[Warehouse<w>] void putDistrictInWarehouse(const int& w, const district_key& key, const district_value & value) {
  int d = (int) key.d_id / DISTRICT_MAX_BATCH_SIZE;
  if( districtIds.count(d) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = getContextName("Warehouse", w);
    mace::string m = getContextName("District", w, d);
    mace::string c = getContextName("DistrictTable", w, d);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );
    
    addNewOwnerships(newOwnerships);

    districtIds.insert(d);
  }

  putDistrictToTable(w, d, key, value);

}

[DistrictTable<wId, dId>] void putDistrictToTable(const int& wId, const int& cdId, district_key const& key, district_value const& value) {
  table[key] = value;
}

// Oorder table
[Warehouses] void putOorderInWarehouses(const oorder_key& key, const oorder_value & value) {
  int w = (int) key.o_w_id/WAREHOUSE_MAX_BATCH_SIZE;
  if( WarehouseIds.count(w) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", w);
    mace::string c = getContextName(" WarehouseTable", w);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(w);
  }

  putOorderInWarehouse(w, key, value);

}

[Warehouse<w>] void putOorderInWarehouse(const int& w, const oorder_key& key, const oorder_value & value) {
  int d = (int) key.o_d_id / DISTRICT_MAX_BATCH_SIZE;
  if( districtIds.count(d) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = getContextName("Warehouse", w);
    mace::string m = getContextName("District", w, d);
    mace::string c = getContextName("DistrictTable", w, d);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );
    
    addNewOwnerships(newOwnerships);

    districtIds.insert(d);
  }

  putOorderInDistrict(w, d, key, value);

}

[District<w, d>] void putOorderInDistrict(const int& w, const int& d, const oorder_key& key, const oorder_value & value) {
  int x = (int) key.o_id / MAX_BATCH_SIZE;

  if( oorderIds.count(x) == 0 ) {
    mace::string p = getContextName("District", w, d);
    mace::string c = getContextName("OorderTable", w, d, x);

    addNewOwnership(p, c);

    oorderIds.insert(x);
  }

  putOorderToTable(w, d, x, key, value);

}

[OorderTable<w, d, x>] void putOorderToTable(const int& w, const int& d, const int& x, const oorder_key& key, const oorder_value & value) {
  table[key] = value;
}

[Warehouses] oorder_value getOorderValueInWarehouses( oorder_key const& key ) {
  int w = key.o_w_id;
  int cwId = (int) w/WAREHOUSE_MAX_BATCH_SIZE;

  if( warehouseIds.count(cwId) == 0 ) {
    customer_value value;
    value.valid = false;
    return value;
  }

  return getOorderValueInWarehouse( cwId, key )
}

[Warehouse<wId>] oorder_value getOorderValueInWarehouse( const int32_t& wId, oorder_key const& key ) {
  int w = key.o_w_id;
  int cwId = (int) w/WAREHOUSE_MAX_BATCH_SIZE;
  ASSER( cwId == wId );

  int d = key.o_d_id;
  int cdId = (int) d/ DISTRICT_MAX_BATCH_SIZE;

  if( districtIds.count(cdId) == 0 ) {
    customer_value value;
    value.valid = false;
    return value;
  }

  return getOorderValueInDistrict( cwId, cdId, key )
}

[District<wId, dId>] oorder_value getOorderValueInWarehouse( const int32_t& wId, const int32_t& dId, oorder_key const& key ) {
  int cwId = (int) key.o_w_id/WAREHOUSE_MAX_BATCH_SIZE;
  int cdId = (int) key.o_d_id/ DISTRICT_MAX_BATCH_SIZE;
  ASSERT( cwId == wId );
  ASSERT( cdId == dId );
  
  int coId =(int) key.o_id / MAX_BATCH_SIZE;

  if( oorderIds.count(coId) == 0 ) {
    oorder_value value;
    value.valid = false;
    return value;
  }

  return getOorderValueFromTable( cwId, cdId, ccId, key )
}

[OorderTable<wId, dId, oId>] oorder_value getOorderValueFromTable(const int& wId, const int& dId, const int& oId, oorder_key const& key) {
  mace::map<oorder_key, oorder_value>::const_iterator iter = table.find(key);
  if( iter == table.end() ) {
    oorder_value value;
    value.valid = false;
    return value;
  } else {
    return iter->second;
  }
}


// oorder_c_id_idx
[Warehouses] void putOorderCIdIdxInWarehouses(const oorder_c_id_idx_key& key, const oorder_c_id_idx_value & value) {
  int w = (int) key.o_w_id/WAREHOUSE_MAX_BATCH_SIZE;
  if( WarehouseIds.count(w) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", w);
    mace::string c = getContextName(" WarehouseTable", w);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(w);
  }

  putOorderCIdIdxInWarehouse(w, key, value);

}

[Warehouse<w>] void putOorderCIdIdxInWarehouse(const int& w, const oorder_c_id_idx_key& key, const oorder_c_id_idx_value & value) {
  int d = (int) key.o_d_id / DISTRICT_MAX_BATCH_SIZE;
  if( districtIds.count(d) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = getContextName("Warehouse", w);
    mace::string m = getContextName("District", w, d);
    mace::string c = getContextName("DistrictTable", w, d);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );
    
    addNewOwnerships(newOwnerships);

    districtIds.insert(d);
  }

  putOorderCIdIdxInDistrict(w, d, key, value);

}

[District<w, d>] void putOorderCIdIdxInDistrict(const int& w, const int& d, const oorder_c_id_idx_key& key, const oorder_c_id_idx_value & value) {
  int x = (int) key.o_c_id / MAX_BATCH_SIZE;

  if( oorderCIdIdxIds.count(x) == 0 ) {
    mace::string p = getContextName("District", w, d);
    mace::string c = getContextName("OorderCIdIdxTable", w, d, x);

    addNewOwnership(p, c);

    oorderCIdIdxIds.insert(x);
  }

  putOorderCIdIdxToTable(w, d, x, key, value);

}

[OorderCIdIdxTable<w, d, x>] void putOorderCIdIdxToTable(const int& w, const int& d, const int& x, const oorder_c_id_idx_key& key, const oorder_c_id_idx_value & value) {
  table[key] = value;
}

[District<w, d>] mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value> scanOorderCIdIdxInDistrict(const int& w, const int& d, const oorder_c_id_idx_key& low, const oorder_c_id_idx_key& high) {
  ASSERT(low.o_w_id == high.o_w_id && low.o_d_id == high.o_d_id);
  
  uint32_t lc = low.o_c_id;
  uint32_t hc = high.o_c_id;
  uint32_t lo = low.o_o_id;
  uint32_t ho = high.o_o_id;

  ASSERT(lc <= hc);
  ASSERT(lo <= ho);
  
  int lx = (int) lc / MAX_BATCH_SIZE;
  int hx = (int) hc / MAX_BATCH_SIZE;


  mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value> scan_pairs;
  for(int i=lx; i<=hx; i++) {
    if(oorderCIdIdxIds.count(i) == 0) {
      continue;
    }
    mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value> s = scanOorderCIdIdxInTable(w, d, i, lc, hc, lo, ho);

    mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value>::const_iterator iter = s.begin();
    for(; iter!=s.end(); iter++) {
      scan_pairs[iter->first] = iter->second;
    }
  }

  return scan_pairs;
}

[OorderCIdIdxTable<w, d, x>] mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value> scanOorderCIdIdxInTable(const int& w, const int& d, const int& x, const int& lc, const int& hc, const int& lo, const int& ho) {
  mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value> scan_pairs;

  mace::map<oorder_c_id_idx_key, oorder_c_id_idx_value>::const_iterator iter = table.begin();
  for(; iter!=table.end(); iter++) {
    const oorder_c_id_idx_key& k = iter->first;
    if( k.o_o_id >= lo && k.o_o_id <= ho && k.o_c_id >= lc && k.o_c_id <= hc){
      scan_pairs[iter->first] = iter->second;
    }
  }
  return scan_pairs;
}

// Customer table
[Warehouses] customer_value getCustomerValueInWarehouses( customer_key const& key ) {
  int w = key.c_w_id;
  int cwId = (int) w/WAREHOUSE_MAX_BATCH_SIZE;

  if( warehouseIds.count(cwId) == 0 ) {
    customer_value value;
    value.valid = false;
    return value;
  }

  return getCustomerValueInWarehouse( cwId, key )
}

[Warehouse<wId>] customer_value getCustomerValueInWarehouse( const int32_t& wId, customer_key const& key ) {
  int w = key.c_w_id;
  int cwId = (int) w/WAREHOUSE_MAX_BATCH_SIZE;
  ASSER( cwId == wId );

  int d = key.c_d_id;
  int cdId = (int) d/ DISTRICT_MAX_BATCH_SIZE;

  if( districtIds.count(cdId) == 0 ) {
    customer_value value;
    value.valid = false;
    return value;
  }

  return getCustomerValueInDistrict( cwId, cdId, key )
}

[District<wId, dId>] customer_value getCustomerValueInWarehouse( const int32_t& wId, const int32_t& dId, customer_key const& key ) {
  int cwId = (int) key.c_w_id/WAREHOUSE_MAX_BATCH_SIZE;
  int cdId = (int) key.c_d_id/ DISTRICT_MAX_BATCH_SIZE;
  ASSERT( cwId == wId );
  ASSERT( cdId == dId );
  
  int ccId =(int) key.c_id / MAX_BATCH_SIZE;

  if( customerIds.count(ccId) == 0 ) {
    customer_value value;
    value.valid = false;
    return value;
  }

  return getCustomerValueFromTable( cwId, cdId, ccId, key )
}

[CustomerTable<wId, dId, cId>] customer_value getCustomerValueFromTable(const int& wId, const int& dId, const int& cId, customer_key const& key) {
  mace::map<customer_key, customer_value>::const_iterator iter = c_table.find(key);
  if( iter == c_table.end() ) {
    customer_value value;
    value.valid = false;
    return value;
  } else {
    return iter->second;
  }
}

[Warehouses] void putCustomerInWarehouses(const customer_key& key, const customer_value & value) {
  int w = (int) key.c_w_id/WAREHOUSE_MAX_BATCH_SIZE;
  if( WarehouseIds.count(w) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", w);
    mace::string c = getContextName(" WarehouseTable", w);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(w);
  }

  putCustomerWarehouse(w, key, value);

}

[Warehouse<w>] void putCustomerInWarehouse(const int& w, const customer_key& key, const customer_value & value) {
  int d = (int) key.c_d_id / DISTRICT_MAX_BATCH_SIZE;
  if( districtIds.count(d) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = getContextName("Warehouse", w);
    mace::string m = getContextName("District", w, d);
    mace::string c = getContextName("DistrictTable", w, d);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );
    
    addNewOwnerships(newOwnerships);

    districtIds.insert(d);
  }

  putCustomerInDistrict(w, d, key, value);

}

[District<w, d>] void putCustomerDistrict(const int& w, const int& d, const customer_key& key, const customer_value & value) {
  int x = (int) key.c_id / MAX_BATCH_SIZE;

  if( customerIds.count(x) == 0 ) {
    mace::string p = getContextName("District", w, d);
    mace::string c = getContextName("CustomerTable", w, d, x);

    addNewOwnership(p, c);

    customerIds.insert(x);
  }

  putCustomerToTable(w, d, x, key, value);

}

[CustomerTable<w, d, x>] void putCustomerToTable(const int& w, const int& d, const int& x, const customer_key& key, const customer_value & value) {
  c_table[key] = value;
}


// Item table
[Items] item_value getItemValueInItems( item_key const& key ) {
  int ciId = (int)  key.i_id / MAX_BATCH_SIZE;

  if( itemIds.count(ciId) == 0 ) {
    item_value value;
    value.valid = false;
    return value;
  }

  return getItemValueFromTable( ciId, key );
}

[ItemTable<iId>] item_value getCustomerValueFromTable(const int& iId, item_key const& key) {
  mace::map<item_key, item_value>::const_iterator iter = table.find(key);
  if( iter == table.end() ) {
    item_value value;
    value.valid = false;
    return value;
  } else {
    return iter->second;
  }
}



// History table
[Warehouses] void putHistoryInWarehouses(const history_key& key, const history_value & value) {
  int w = (int) key.h_c_w_id/WAREHOUSE_MAX_BATCH_SIZE;
  if( warehouseIds.count(w) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", w);
    mace::string c = getContextName(" WarehouseTable", w);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(w);
  }

  putHistoryWarehouse(w, key, value);

}

[Warehouse<w>] void putHistoryInWarehouse(const int& w, const history_key& key, const history_value & value) {
  int d = (int) key.h_c_d_id / DISTRICT_MAX_BATCH_SIZE;
  if( districtIds.count(d) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = getContextName("Warehouse", w);
    mace::string m = getContextName("District", w, d);
    mace::string c = getContextName("DistrictTable", w, d);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );
    
    addNewOwnerships(newOwnerships);

    districtIds.insert(d);
  }

  putHistoryInDistrict(w, d, key, value);

}

[District<w, d>] void putHistoryDistrict(const int& w, const int& d, const history_key& key, const history_value & value) {
  int x = (int) key.h_c_id / MAX_BATCH_SIZE;

  if( customerIds.count(x) == 0 ) {
    mace::string p = getContextName("District", w, d);
    mace::string c = getContextName("CustomerTable", w, d, x);

    addNewOwnership(p, c);

    customerIds.insert(x);
  }

  putHistoryToTable(w, d, x, key, value);

}

[CustomerTable<w, d, x>] void putHistoryToTable(const int& w, const int& d, const int& x, const history_key& key, const history_value & value) {
  h_table[key] = value;
}


// Stock table
[Warehouses] stock_value getStockValueInWarehouses( stock_key const& key ) {
  int w = key.s_w_id;
  int cwId = (int) w/WAREHOUSE_MAX_BATCH_SIZE;

  if( warehouseIds.count(cwId) == 0 ) {
    stock_value value;
    value.valid = false;
    return value;
  }

  return getStockValueInWarehouse( cwId, key )
}

[Warehouse<wId>] stock_value getStockValueInWarehouse( const int32_t& wId, stock_key const& key ) {
  int w = key.s_w_id;
  int cwId = (int) w/WAREHOUSE_MAX_BATCH_SIZE;
  ASSER( cwId == wId );

  int s = key.s_i_id;
  int csId = (int) s/ MAX_BATCH_SIZE;

  if( stockIds.count(csId) == 0 ) {
    stock_value value;
    value.valid = false;
    return value;
  }

  return getStockValueFromTable( cwId, csId, key )
}

[StockTable<wId, sId>] stock_value getStockValueFromTable(const int& wId, const int& sId, stock_key const& key) {
  mace::map<stock_key, stock_value>::const_iterator iter = table.find(key);
  if( iter == table.end() ) {
    stock_value value;
    value.valid = false;
    return value;
  } else {
    return iter->second;
  }
}

[Warehouses] void putStockInWarehouses(stock_key const& key, stock_value const& value) {
  int cwId = (int) key.s_w_id / WAREHOUSE_MAX_BATCH_SIZE;
  
  if( WarehouseIds.count(cwId) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", cwId);
    mace::string c = getContextName(" WarehouseTable", cwId);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(w);
  }

  putStockInWarehouse(cwId, key, value);

}

[Warehouse<w>] void putStockInWarehouse(const int& w, stock_key const& key, stock_value const& value) {
  int csId = (int) key.s_i_id / MAX_BATCH_SIZE; 

  if( stockIds.count(csId) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = getContextName("Warehouse", w);
    mace::string c = getContextName("StocktTable", w, csId);

    addNewOwnership(p, c);

    stockIds.insert(csId);
  }

  putStockToTable(w, csId, key, value);

}

[StockTable<w, s>] void putStockTable(const int& w, const int& s, stock_key const& key, stock_value const& value) {
  table[key] = value;
}

// Order_line table
[Warehouses] void putOrderLineInWarehouses(const order_line_key& key, const order_line_value & value) {
  int w = (int) key.ol_w_id/WAREHOUSE_MAX_BATCH_SIZE;
  if( WarehouseIds.count(w) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = "Warehouses";
    mace::string m = getContextName("Warehouse", w);
    mace::string c = getContextName(" WarehouseTable", w);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );

    addNewOwnerships(newOwnerships);

    warehousesIds.insert(w);
  }

  putOrderLinenWarehouse(w, key, value);

}

[Warehouse<w>] void putOrderLineInWarehouse(const int& w, const order_line_key& key, const order_line_value & value) {
  int d = (int) key.ol_d_id / DISTRICT_MAX_BATCH_SIZE;
  if( districtIds.count(d) == 0 ) {
    mace::set< mace::pair<mace::string, mace::string> > newOwnerships;

    mace::string p = getContextName("Warehouse", w);
    mace::string m = getContextName("District", w, d);
    mace::string c = getContextName("DistrictTable", w, d);

    newOwnerships.insert( make_pair(p, m) );
    newOwnerships.insert( mace_pair(m, c) );
    
    addNewOwnerships(newOwnerships);

    districtIds.insert(d);
  }

  putOrderLineInDistrict(w, d, key, value);

}

[District<w, d>] void putOrderLineInDistrict(const int& w, const int& d, const order_line_key& key, const order_line_value & value) {
  int x = (int) key.ol_o_id / MAX_BATCH_SIZE;

  if( orderLineIds.count(x) == 0 ) {
    mace::string p = getContextName("District", w, d);
    mace::string c = getContextName("OrderLineTable", w, d, x);

    addNewOwnership(p, c);

    oorderIds.insert(x);
  }

  putOrderLineToTable(w, d, x, key, value);

}

[OrderLineTable<w, d, x>] void putOrderLineToTable(const int& w, const int& d, const int& x, const order_line_key& key, const order_line_value & value) {
  table[key] = value;
}

[District<wId, dId>] mace::map<order_line_key, order_line_value> scanOrderLineInDistrict(const int& wId, const int& dId, const order_line_key& low, const order_line_key& high) {
  ASSERT(low.ol_w_id == high.ol_w_id && low.ol_d_id == high.ol_d_id);
  uint32_t w = low.ol_w_id;
  uint32_t d = low.ol_d_id;

  uint32_t lo = low.ol_o_id;
  uint32_t ho = high.ol_o_id;
  uint32_t lnum = low.ol_number;
  uint32_t hnum = high.ol_number;

  ASSERT(lo <= ho);
  ASSERT(lnum <= hnum);

  int lx = (int) lo / MAX_BATCH_SIZE;
  int hx = (int) ho / MAX_BATCH_SIZE;

  mace::map<order_line_key, order_line_value> scan_pairs;
  for(int i=lx; i<=hx; i++) {
    if( orderLineIds.count(i) == 0) {
      continue;
    }
    mace::map<order_line_key, order_line_value> s = scanOrderLineInTable(w, d, i, lo, ho, lnum, hnum);

    mace::map<order_line_key, order_line_value>::const_iterator iter = s.begin();
    for(; iter!=s.end(); iter++) {
      scan_pairs[iter->first] = iter->second;
    }
  }

  return scan_pairs;
}

[OrderLineTable<w, d, x>] mace::map<order_line_key, order_line_value> scanOrderLineInTable(const int& w, const int& d, const int& x, const int& lo, const int& ho, const int& lnum, const int& hnum) {
  mace::map<order_line_key, order_line_value> scan_pairs;

  mace::map<order_line_key, order_line_value>::const_iterator iter = table.begin();
  for(; iter!=table.end(); iter++) {
    const order_line_key& k = iter->first;
    bool flag = false;
    if( ho > lo ) {
      if(k.ol_o_id >= lo && k.ol_o_id <= ho) {
        flag = true;
      }
    } else if( ho == lo ) {
      if(k.ol_number >= lnum && k.ol_number <= hnum) {
        flag = true;
      }
    }
    if( flag ){
      scan_pairs[iter->first] = iter->second;
    }
  }
  return scan_pairs;
}

// Customer_name_idx
[Warehouse<wId>] mace::map<customer_name_idx_key, customer_name_idx_value> scanCustomerNameIdxInWarehouse(const int& wId, const customer_name_idx_key& low, const customer_name_idx_key& high) {
  ASSERT(low.c_w_id == high.c_w_id);
  uint32_t w = low.c_w_id;

  uint32_t ld = low.c_d_id;
  uint32_t hd = high.c_d_id;

  ASSERT(ld <= hd);

  int lx = (int) ld / MAX_BATCH_SIZE;
  int hx = (int) hd / MAX_BATCH_SIZE;

  mace::map<customer_name_idx_key, customer_name_idx_value> scan_pairs;
  for(uint32_t i=ld; i<=hd; i++) {
    if( customerNameIdxIds.count(i) == 0 ) {
      continue;
    }
    mace::map<customer_name_idx_key, customer_name_idx_value> s = scanCustomerNameIdxInTable(w, i, lx, hx);

    mace::map<customer_name_idx_key, customer_name_idx_value>::const_iterator iter = s.begin();
    for(; iter!=s.end(); iter++) {
      scan_pairs[iter->first] = iter->second;
    }
  }

  return scan_pairs;
}

[CustomerNameIdxTable<w, t>] mace::map<customer_name_idx_key, customer_name_idx_value> scanCustomerNameIdxInTable(const int& w, const int& t, const int& l, const int& h) {
  mace::map<customer_name_idx_key, customer_name_idx_value> scan_pairs;

  mace::map<customer_name_idx_key, customer_name_idx_value>::const_iterator iter = table.begin();
  for(; iter!=table.end(); iter++) {
    const customer_name_idx_key& k = iter->first;
    if( k.c_d_id >= l && k.c_d_id <= h){
      scan_pairs[iter->first] = iter->second;
    }
  }
  return scan_pairs;
}

[__null] void initCustomerNameIdxValue(customer_name_idx_value& n_v, const customer_name_idx_value& o_v) {
  n_v.c_id = o_v.c_id;
}