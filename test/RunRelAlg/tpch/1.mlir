//RUN: db-run-query %s %S/../../../resources/data/tpch | FileCheck %s
//CHECK: |                  l_returnflag  |                  l_linestatus  |                       sum_qty  |                sum_base_price  |                sum_disc_price  |                    sum_charge  |                       avg_qty  |                     avg_price  |                      avg_disc  |                   count_order  |
//CHECK: -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//CHECK: |                             A  |                             F  |                    3774200.00  |                 5320753880.69  |                 5054095661.43  |                 5256750064.91  |                         25.53  |                      36002.12  |                          0.05  |                        147790  |
//CHECK: |                             N  |                             F  |                      95257.00  |                  133737795.84  |                  127132357.30  |                  132286258.95  |                         25.30  |                      35521.32  |                          0.04  |                          3765  |
//CHECK: |                             N  |                             O  |                    7459297.00  |                10512270008.90  |                 9986237142.30  |                10385575878.94  |                         25.54  |                      36000.92  |                          0.05  |                        292000  |
//CHECK: |                             R  |                             F  |                    3785523.00  |                 5337950526.47  |                 5071817924.80  |                 5274404231.65  |                         25.52  |                      35994.02  |                          0.04  |                        148301  |
module @querymodule{
    func  @main ()  -> !db.table{
        %1 = relalg.basetable @lineitem { table_identifier="lineitem", rows=600572 , pkey=["l_orderkey","l_linenumber"]} columns: {l_orderkey => @l_orderkey({type=!db.int<32>}),
            l_partkey => @l_partkey({type=!db.int<32>}),
            l_suppkey => @l_suppkey({type=!db.int<32>}),
            l_linenumber => @l_linenumber({type=!db.int<32>}),
            l_quantity => @l_quantity({type=!db.decimal<15,2>}),
            l_extendedprice => @l_extendedprice({type=!db.decimal<15,2>}),
            l_discount => @l_discount({type=!db.decimal<15,2>}),
            l_tax => @l_tax({type=!db.decimal<15,2>}),
            l_returnflag => @l_returnflag({type=!db.char<1>}),
            l_linestatus => @l_linestatus({type=!db.char<1>}),
            l_shipdate => @l_shipdate({type=!db.date<day>}),
            l_commitdate => @l_commitdate({type=!db.date<day>}),
            l_receiptdate => @l_receiptdate({type=!db.date<day>}),
            l_shipinstruct => @l_shipinstruct({type=!db.string}),
            l_shipmode => @l_shipmode({type=!db.string}),
            l_comment => @l_comment({type=!db.string})
        }
        %3 = relalg.selection %1(%2: !relalg.tuple) {
            %4 = relalg.getattr %2 @lineitem::@l_shipdate : !db.date<day>
            %5 = db.constant ("1998-12-01") :!db.date<day>
            %6 = db.constant ("7776000000") :!db.interval<daytime>
            %7 = db.date_sub %5 : !db.date<day>,%6 : !db.interval<daytime>
            %8 = db.compare lte %4 : !db.date<day>,%7 : !db.date<day>
            relalg.return %8 : !db.bool
        }
        %10 = relalg.map @map %3 (%9: !relalg.tuple) {
            %11 = relalg.getattr %9 @lineitem::@l_extendedprice : !db.decimal<15,2>
            %12 = db.constant (1) :!db.decimal<15,2>
            %13 = relalg.getattr %9 @lineitem::@l_discount : !db.decimal<15,2>
            %14 = db.sub %12 : !db.decimal<15,2>,%13 : !db.decimal<15,2>
            %15 = db.mul %11 : !db.decimal<15,2>,%14 : !db.decimal<15,2>
            %16 = relalg.addattr %9, @aggfmname3({type=!db.decimal<15,2>}) %15
            %17 = relalg.getattr %9 @lineitem::@l_extendedprice : !db.decimal<15,2>
            %18 = db.constant (1) :!db.decimal<15,2>
            %19 = relalg.getattr %9 @lineitem::@l_discount : !db.decimal<15,2>
            %20 = db.sub %18 : !db.decimal<15,2>,%19 : !db.decimal<15,2>
            %21 = db.constant (1) :!db.decimal<15,2>
            %22 = relalg.getattr %9 @lineitem::@l_tax : !db.decimal<15,2>
            %23 = db.add %21 : !db.decimal<15,2>,%22 : !db.decimal<15,2>
            %24 = db.mul %17 : !db.decimal<15,2>,%20 : !db.decimal<15,2>
            %25 = db.mul %23 : !db.decimal<15,2>,%24 : !db.decimal<15,2>
            %26 = relalg.addattr %16, @aggfmname5({type=!db.decimal<15,2>}) %25
            relalg.return %26 : !relalg.tuple
        }
        %29 = relalg.aggregation @aggr %10 [@lineitem::@l_returnflag,@lineitem::@l_linestatus] (%27 : !relalg.tuplestream, %28 : !relalg.tuple) {
            %30 = relalg.aggrfn sum @lineitem::@l_quantity %27 : !db.decimal<15,2>
            %31 = relalg.addattr %28, @aggfmname1({type=!db.decimal<15,2>}) %30
            %32 = relalg.aggrfn sum @lineitem::@l_extendedprice %27 : !db.decimal<15,2>
            %33 = relalg.addattr %31, @aggfmname2({type=!db.decimal<15,2>}) %32
            %34 = relalg.aggrfn sum @map::@aggfmname3 %27 : !db.decimal<15,2>
            %35 = relalg.addattr %33, @aggfmname4({type=!db.decimal<15,2>}) %34
            %36 = relalg.aggrfn sum @map::@aggfmname5 %27 : !db.decimal<15,2>
            %37 = relalg.addattr %35, @aggfmname6({type=!db.decimal<15,2>}) %36
            %38 = relalg.aggrfn avg @lineitem::@l_quantity %27 : !db.decimal<15,2>
            %39 = relalg.addattr %37, @aggfmname7({type=!db.decimal<15,2>}) %38
            %40 = relalg.aggrfn avg @lineitem::@l_extendedprice %27 : !db.decimal<15,2>
            %41 = relalg.addattr %39, @aggfmname8({type=!db.decimal<15,2>}) %40
            %42 = relalg.aggrfn avg @lineitem::@l_discount %27 : !db.decimal<15,2>
            %43 = relalg.addattr %41, @aggfmname9({type=!db.decimal<15,2>}) %42
            %44 = relalg.count %27
            %45 = relalg.addattr %43, @aggfmname10({type=!db.int<64>}) %44
            relalg.return %45 : !relalg.tuple
        }
        %46 = relalg.sort %29 [(@lineitem::@l_returnflag,asc),(@lineitem::@l_linestatus,asc)]
        %47 = relalg.materialize %46 [@lineitem::@l_returnflag,@lineitem::@l_linestatus,@aggr::@aggfmname1,@aggr::@aggfmname2,@aggr::@aggfmname4,@aggr::@aggfmname6,@aggr::@aggfmname7,@aggr::@aggfmname8,@aggr::@aggfmname9,@aggr::@aggfmname10] => ["l_returnflag","l_linestatus","sum_qty","sum_base_price","sum_disc_price","sum_charge","avg_qty","avg_price","avg_disc","count_order"] : !db.table
        return %47 : !db.table
    }
}

