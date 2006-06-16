
#include <sys/time.h>



/* tpch_l_shipmode.
   TODO: Unnecessary */
enum tpch_l_shipmode {
    REG_AIR,
    AIR,
    RAIL,
    TRUCK,
    MAIL,
    FOB,
    SHIP
};


/* Lineitem tuple.
   TODO: Catalog will provide this metadata info */
struct tpch_lineitem_tuple {
    int L_ORDERKEY;
    int L_PARTKEY;
    int L_SUPPKEY;
    int L_LINENUMBER;
    double L_QUANTITY;
    double L_EXTENDEDPRICE;
    double L_DISCOUNT;
    double L_TAX;
    char L_RETURNFLAG;
    char L_LINESTATUS;
    time_t L_SHIPDATE;
    time_t L_COMMITDATE;
    time_t L_RECEIPTDATE;
    char L_SHIPINSTRUCT[25];
    tpch_l_shipmode L_SHIPMODE;
    // char L_COMMENT[44];
};


int main() {

    //  struct tpch_lineitem_tuple tup;

  while (1);
  
  return 0;
}
