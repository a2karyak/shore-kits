
/* tpch_setup.cpp */
/* Setups the tpch dataset/workload */

#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

/* BerkeleyDB */
#include <db_cxx.h>


/* Sets the size of the Bufferpool */

/* Set Bufferpool equal to 450 MB -- Maximum is 4GB in 32-bit platforms */
size_t TPCH_BUFFER_POOL_SIZE_GB = 0; /* 0 GB */
size_t TPCH_BUFFER_POOL_SIZE_BYTES = 450 * 1024 * 1024; /* 450 MB */





/* Sets bufferpool equal to 2GB + 128MB - example value */
/* size_t TPCH_BUFFER_POOL_SIZE_GB = 2; */
/* size_t TPCH_BUFFER_POOL_SIZE_BYTES = 128 * 1024 * 1024; */

#define MAX_LINE_LENGTH 1024
char linebuffer[MAX_LINE_LENGTH];

/* Monitor related */
void register_table(const char* data_dir, char* table_id, char* table_name);





// btree comparison functions for all tpc-h tables





void tpch_load_customer_table(Db* db, const char* fname) {
	printf("Populating CUSTOMER...\n");
	tpch_customer_tuple tup;

	FILE* fd = fopen(fname, "r");
	if (!fd) {
		fprintf(stderr, "Enable to open %s - bailing out\n", fname);
		exit(1);
	}
	while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
		// clear the tuple
		memset(&tup, 0, sizeof(tup));

		// split line into tab separated parts
		char* tmp = strtok(linebuffer, "|");
		tup.C_CUSTKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.C_NAME, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		memcpy(tup.C_ADDRESS, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		tup.C_NATIONKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.C_PHONE, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		tup.C_ACCTBAL = atof(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.C_MKTSEGMENT, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		// memcpy(tup.C_COMMENT, tmp, strlen(tmp));

		// insert tuple into database
		Dbt key(&tup.C_CUSTKEY, sizeof(int));
		Dbt data(&tup, sizeof(tup));
		db->put(NULL, &key, &data, 0);

	}
	fclose(fd);
	printf("Done\n");
}

void tpch_load_lineitem_table(Db* db, const char* fname) {
	printf("Populating LINEITEM...\n");
	tpch_lineitem_tuple tup;

	FILE* fd = fopen(fname, "r");
	if (!fd) {
		fprintf(stderr, "Unable to open %s - bailing out\n", fname);
		exit(1);
	}
	while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
		// clear the tuple
		memset(&tup, 0, sizeof(tup));

		// split line into tab separated parts
		char* tmp = strtok(linebuffer, "|");
		tup.L_ORDERKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.L_PARTKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.L_SUPPKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.L_LINENUMBER = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.L_QUANTITY = atof(tmp);
		tmp = strtok(NULL, "|");
		tup.L_EXTENDEDPRICE = atof(tmp);
		tmp = strtok(NULL, "|");
		tup.L_DISCOUNT = atof(tmp);
		tmp = strtok(NULL, "|");
		tup.L_TAX = atof(tmp);
		tmp = strtok(NULL, "|");
		tup.L_RETURNFLAG = tmp[0];
		tmp = strtok(NULL, "|");
		tup.L_LINESTATUS = tmp[0];
		tmp = strtok(NULL, "|");
		tup.L_SHIPDATE = datestr_to_timet(tmp);
		tmp = strtok(NULL, "|");
		tup.L_COMMITDATE = datestr_to_timet(tmp);
		tmp = strtok(NULL, "|");
		tup.L_RECEIPTDATE = datestr_to_timet(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.L_SHIPINSTRUCT, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		tup.L_SHIPMODE = modestr_to_shipmode(tmp);
		tmp = strtok(NULL, "|");
		// memcpy(tup.L_COMMENT, tmp, strlen(tmp));

		// insert tuple into database
		// key is composed of 3 fields 	L_ORDERKEY, L_PARTKEY, and L_SUPPKEY
		Dbt key(&tup.L_ORDERKEY, 3 * sizeof(int));
		Dbt data(&tup, sizeof(tup));
		db->put(NULL, &key, &data, 0);

	}
	fclose(fd);
	printf("Done\n");
}

void tpch_load_nation_table(Db* db, const char* fname) {
	printf("Populating NATION...\n");
	tpch_nation_tuple tup;

	FILE* fd = fopen(fname, "r");
	if (!fd) {
		fprintf(stderr, "Unable to open %s - bailing out\n", fname);
		exit(1);
	}
	while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
		// clear the tuple
		memset(&tup, 0, sizeof(tup));

		// split line into tab separated parts
		char* tmp = strtok(linebuffer, "|");
		tup.N_NATIONKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.N_NAME = nnamestr_to_nname(tmp);
		tmp = strtok(NULL, "|");
		tup.N_REGIONKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		// memcpy(tup.N_COMMENT, tmp, strlen(tmp));

		// insert tuple into database
		Dbt key(&tup.N_NATIONKEY, sizeof(int));
		Dbt data(&tup, sizeof(tup));
		db->put(NULL, &key, &data, 0);

	}
	fclose(fd);
	printf("Done\n");
}

void tpch_load_orders_table(Db* db, const char* fname) {
	printf("Populating ORDERS...\n");
	tpch_orders_tuple tup;

	FILE* fd = fopen(fname, "r");
	if (!fd) {
		fprintf(stderr, "Unable to open %s - bailing out\n", fname);
		exit(1);
	}
	while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
		// clear the tuple
		memset(&tup, 0, sizeof(tup));

		// split line into tab separated parts
		char* tmp = strtok(linebuffer, "|");
		tup.O_ORDERKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.O_CUSTKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.O_ORDERSTATUS = tmp[0];
		tmp = strtok(NULL, "|");
		tup.O_TOTALPRICE = atof(tmp);
		tmp = strtok(NULL, "|");
		tup.O_ORDERDATE = datestr_to_timet(tmp);
		tmp = strtok(NULL, "|");
		tup.O_ORDERPRIORITY = prioritystr_to_orderpriorty(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.O_CLERK, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		tup.O_SHIPPRIORITY = atoi(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.O_COMMENT, tmp, strlen(tmp));

		// insert tuple into database
		// key is composed of 2 fields 	O_ORDERKEY and O_CUSTKEY
		Dbt key(&tup.O_ORDERKEY, 2 * sizeof(int));
		Dbt data(&tup, sizeof(tup));
		db->put(NULL, &key, &data, 0);

	}
	fclose(fd);
	printf("Done\n");
}

void tpch_load_part_table(Db* db, const char* fname) {
	printf("Populating PART...\n");
	tpch_part_tuple tup;

	FILE* fd = fopen(fname, "r");
	if (!fd) {
		fprintf(stderr, "Unable to open %s - bailing out\n", fname);
		exit(1);
	}
	while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
		// clear the tuple
		memset(&tup, 0, sizeof(tup));

		// split line into tab separated parts
		char* tmp = strtok(linebuffer, "|");
		tup.P_PARTKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.P_NAME, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		memcpy(tup.P_MFGR, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		memcpy(tup.P_BRAND, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		memcpy(tup.P_TYPE, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		tup.P_SIZE = atoi(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.P_CONTAINER, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		tup.P_RETAILPRICE = atof(tmp);
		tmp = strtok(NULL, "|");
		// memcpy(tup.P_COMMENT, tmp, strlen(tmp));

		// insert tuple into database
		Dbt key(&tup.P_PARTKEY, sizeof(int));
		Dbt data(&tup, sizeof(tup));
		db->put(NULL, &key, &data, 0);

	}
	fclose(fd);
	printf("Done\n");
}

void tpch_load_partsupp_table(Db* db, const char* fname) {
	printf("Populating PARTSUPP...\n");
	tpch_partsupp_tuple tup;

	FILE* fd = fopen(fname, "r");
	if (!fd) {
		fprintf(stderr, "Unable to open %s - bailing out\n", fname);
		exit(1);
	}
	while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
		// clear the tuple
		memset(&tup, 0, sizeof(tup));

		// split line into tab separated parts
		char* tmp = strtok(linebuffer, "|");
		tup.PS_PARTKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.PS_SUPPKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.PS_AVAILQTY = atoi(tmp);
		tmp = strtok(NULL, "|");
		tup.PS_SUPPLYCOST = atof(tmp);
		tmp = strtok(NULL, "|");
		// memcpy(tup.PS_COMMENT, tmp, strlen(tmp));
	
		// insert tuple into database
		// key is composed of 2 fields 	PS_PARTKEY and PS_SUPPKEY
		Dbt key(&tup.PS_PARTKEY, 2 * sizeof(int));
		Dbt data(&tup, sizeof(tup));
		db->put(NULL, &key, &data, 0);

	}
	fclose(fd);
	printf("Done\n");
}

void tpch_load_region_table(Db* db, const char* fname) {
	printf("Populating REGION...\n");
	tpch_region_tuple tup;

	FILE* fd = fopen(fname, "r");
	if (!fd) {
		fprintf(stderr, "Unable to open %s - bailing out\n", fname);
		exit(1);
	}
	while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
		// clear the tuple
		memset(&tup, 0, sizeof(tup));

		// split line into tab separated parts
		char* tmp = strtok(linebuffer, "|");
		tup.R_REGIONKEY= atoi(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.R_NAME, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		// memcpy(tup.R_COMMENT, tmp, strlen(tmp));

		// insert tuple into database
		Dbt key(&tup.R_REGIONKEY, sizeof(int));
		Dbt data(&tup, sizeof(tup));
		db->put(NULL, &key, &data, 0);

	}
	fclose(fd);
	printf("Done\n");
}

void tpch_load_supplier_table(Db* db, const char* fname) {
	printf("Populating SUPPLIER...\n");
	tpch_supplier_tuple tup;

	FILE* fd = fopen(fname, "r");
	if (!fd) {
		fprintf(stderr, "Unable to open %s - bailing out\n", fname);
		exit(1);
	}
	while (fgets(linebuffer, MAX_LINE_LENGTH, fd)) {
		// clear the tuple
		memset(&tup, 0, sizeof(tup));

		// split line into tab separated parts
		char* tmp = strtok(linebuffer, "|");
		tup.S_SUPPKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.S_NAME, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		memcpy(tup.S_ADDRESS, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		tup.S_NATIONKEY = atoi(tmp);
		tmp = strtok(NULL, "|");
		memcpy(tup.S_PHONE, tmp, strlen(tmp));
		tmp = strtok(NULL, "|");
		tup.S_ACCTBAL= atof(tmp);
		tmp = strtok(NULL, "|");
		// memcpy(tup.S_COMMENT, tmp, strlen(tmp));

		// insert tuple into database
		Dbt key(&tup.S_SUPPKEY, sizeof(int));
		Dbt data(&tup, sizeof(tup));
		db->put(NULL, &key, &data, 0);

	}
	fclose(fd);
	printf("Done\n");
}



int setup_tpch(const char *home, const char *data_dir) {

  //
  // Create an environment object and initialize it for error
  // reporting.
  //
  dbenv = new DbEnv(0);
  dbenv->set_errpfx("qpipe");
	
  //
  // We want to specify the shared memory buffer pool cachesize,
  // but everything else is the default.
  //

  
  //dbenv->set_cachesize(0, TPCH_BUFFER_POOL_SIZE, 0);
  if (dbenv->set_cachesize(TPCH_BUFFER_POOL_SIZE_GB, TPCH_BUFFER_POOL_SIZE_BYTES, 0)) {
    fprintf(stdout, "*** Error while trying to set BUFFERPOOL size ***\n");
  }
  else {
    fprintf(stdout, "*** BUFFERPOOL SIZE SET: %d GB + %d B ***\n", TPCH_BUFFER_POOL_SIZE_GB, TPCH_BUFFER_POOL_SIZE_BYTES);
  }


  // Databases are in a subdirectory.
  dbenv->set_data_dir(data_dir);
  
  // set temporary directory
  dbenv->set_tmp_dir(TMP_DIR);

  // Open the environment with no transactional support.
  dbenv->open(home, DB_CREATE | DB_PRIVATE | DB_THREAD | DB_INIT_MPOOL, 0);

  tpch_customer = new Db(dbenv, 0);
  tpch_lineitem = new Db(dbenv, 0);
  tpch_nation = new Db(dbenv, 0);
  tpch_orders = new Db(dbenv, 0);
  tpch_part = new Db(dbenv, 0);
  tpch_partsupp = new Db(dbenv, 0);
  tpch_region = new Db(dbenv, 0);
  tpch_supplier = new Db(dbenv, 0);
  
  tpch_customer->set_bt_compare(tpch_customer_bt_compare_fcn);
  tpch_customer->open(NULL, TABLE_CUSTOMER_NAME, NULL, DB_BTREE,
		      DB_CREATE | DB_TRUNCATE |
		      DB_THREAD, 0644);
  
  tpch_lineitem->set_bt_compare(tpch_lineitem_bt_compare_fcn);
  tpch_lineitem->open(NULL, TABLE_LINEITEM_NAME, NULL, DB_BTREE,
		      DB_CREATE | DB_TRUNCATE |
		      DB_THREAD, 0644);
  
  tpch_nation->set_bt_compare(tpch_nation_bt_compare_fcn);
  tpch_nation->open(NULL, TABLE_NATION_NAME, NULL, DB_BTREE,
		    DB_CREATE | DB_TRUNCATE |
		    DB_THREAD, 0644);
  
  tpch_orders->set_bt_compare(tpch_orders_bt_compare_fcn);
  tpch_orders->open(NULL, TABLE_ORDERS_NAME, NULL, DB_BTREE,
		    DB_CREATE | DB_TRUNCATE |
		    DB_THREAD, 0644);
  
  tpch_part->set_bt_compare(tpch_part_bt_compare_fcn);
  tpch_part->open(NULL, TABLE_PART_NAME, NULL, DB_BTREE,
		  DB_CREATE | DB_TRUNCATE |
		  DB_THREAD, 0644);
  
  tpch_partsupp->set_bt_compare(tpch_partsupp_bt_compare_fcn);
  tpch_partsupp->open(NULL, TABLE_PARTSUPP_NAME, NULL, DB_BTREE,
		      DB_CREATE | DB_TRUNCATE |
		      DB_THREAD, 0644);
  
  tpch_region->set_bt_compare(tpch_region_bt_compare_fcn);
  tpch_region->open(NULL, TABLE_REGION_NAME, NULL, DB_BTREE,
		    DB_CREATE | DB_TRUNCATE |
		    DB_THREAD, 0644);
  
  tpch_supplier->set_bt_compare(tpch_supplier_bt_compare_fcn);
  tpch_supplier->open(NULL, TABLE_SUPPLIER_NAME, NULL, DB_BTREE,
		      DB_CREATE | DB_TRUNCATE |
		      DB_THREAD, 0644);

  // load all TPC-H tables from the files generated by dbgen
  tpch_load_customer_table(tpch_customer, "customer.tbl");
  tpch_load_lineitem_table(tpch_lineitem, "lineitem.tbl");
  tpch_load_nation_table(tpch_nation, "nation.tbl");
  tpch_load_orders_table(tpch_orders, "orders.tbl");
  tpch_load_part_table(tpch_part, "part.tbl");
  tpch_load_partsupp_table(tpch_partsupp, "partsupp.tbl");
  tpch_load_region_table(tpch_region, "region.tbl");
  tpch_load_supplier_table(tpch_supplier, "supplier.tbl");
  
  close_tpch();
	
  return 0;
}



/**
 * close_tpch: Calls the close function for the various tables.
 */
int close_tpch() {

  /**
   * @todo Make sure that the closing procedure is correct
   */

  try {
    

    TRACE_DEBUG("Closing Storage Manager...\n");


    // Close tables
    tpch_customer->close(0);
    tpch_lineitem->close(0);
    tpch_nation->close(0);
    tpch_orders->close(0);
    tpch_part->close(0);
    tpch_partsupp->close(0);
    tpch_region->close(0);
    tpch_supplier->close(0);
    
    // Close the handle.
    dbenv->close(0);
  }
  catch ( DbException &e) {
    TRACE_ERROR("DbException: %s\n", e.what());
  }
  catch ( std::exception &en) {
    TRACE_ERROR("std::exception\n");
  }

    
  /*
    delete dbenv;
    delete tpch_customer;
    delete tpch_lineitem;
    delete tpch_nation;
    delete tpch_orders;
    delete tpch_part;
    delete tpch_partsupp;
    delete tpch_region;
    delete tpch_supplier;
  */
  
  return 0;
}



/**
 * register_table(char* table_id, char* table_name): Sends a corresponding
 * monitor table registration message.
 */

void register_table(const char* data_dir, char* table_id, char* table_name) {

  /* file path */
  char * filepath = (char*)malloc(sizeof(char)*(strlen(data_dir)+strlen(table_name)+2));

  strcpy(filepath, data_dir);
  strcat(filepath, "/");
  strcat(filepath, table_name);

  /* gets the size of the table */
  struct stat sbuf;

  if (stat(filepath,&sbuf) == 0) {

    /* MSG_REGISTER_TABLE message
       ID=%s;N=%s;SZ=%d */

    TRACE_DEBUG("TABLE:%s\tSZ:%ld\n", table_id, sbuf.st_size);

    TRACE_MONITOR("|%d|ID=%s;N=%s;SZ=%ld|", MSG_REGISTER_TABLE, table_id, table_name, sbuf.st_size);
  }
  else {
    TRACE_ERROR("STAT() ERROR. TABLE: %s\n", filepath);

  }

  free (filepath);
}


