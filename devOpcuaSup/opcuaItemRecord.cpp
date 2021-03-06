/*************************************************************************\
* Copyright (c) 2018 ITER Organization.
* This module is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 * OPC UA Item record type
 *
 * Author: Ralph Lange <ralph.lange@gmx.de>
 */

#include <iostream>

#include <dbDefs.h>
#include <errlog.h>
#include <alarm.h>
#include <cantProceed.h>
#include <dbAccess.h>
#include <dbEvent.h>
#include <dbFldTypes.h>
#include <errMdef.h>
#include <menuPost.h>
#include <menuYesNo.h>
#include <recSup.h>
#include <recGbl.h>
#include <special.h>

#include <epicsExport.h>  // defines epicsExportSharedSymbols
#define GEN_SIZE_OFFSET
#include "opcuaItemRecord.h"
#undef GEN_SIZE_OFFSET
#include "devOpcua.h"
#include "ItemUaSdk.h"    //FIXME: replace item creation with factory call (see below)
#include "RecordConnector.h"
#include "linkParser.h"

#include <epicsVersion.h>
#ifdef VERSION_INT
#  if EPICS_VERSION_INT < VERSION_INT(3,16,0,2)
#    define RECSUPFUN_CAST(F) reinterpret_cast<RECSUPFUN>(F)
#  else
#    define RECSUPFUN_CAST(F) F
#  endif
#else
#  define RECSUPFUN_CAST reinterpret_cast<RECSUPFUN>(F)
#endif

namespace {

using namespace DevOpcua;

long readValue(opcuaItemRecord *prec);

#define TRY \
    if (!prec->dpvt) return 0; \
    RecordConnector *pvt = static_cast<RecordConnector*>(prec->dpvt); \
    try
#define CATCH() catch(std::exception& e) { \
    std::cerr << prec->name << " Error : " << e.what() << std::endl; \
    (void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM); \
    return 0; }

void monitor(opcuaItemRecord *);

long
init_record (dbCommon *pdbc, int pass)
{
    opcuaItemRecord *prec = reinterpret_cast<opcuaItemRecord *>(pdbc);
    dbLoadLink(&prec->siml, DBF_USHORT, &prec->simm);

    if (pass == 0) {
        try {
            DBEntry ent(pdbc);
            std::unique_ptr<RecordConnector> pvt (new RecordConnector(pdbc));
            pvt->plinkinfo = parseLink(pdbc, ent);
            ItemUaSdk *pitem = new ItemUaSdk(*pvt->plinkinfo); //FIXME: replace item creation with factory call
            pitem->itemRecord = prec;
            pvt->pitem = pitem;
            prec->dpvt = pvt.release();
            std::cout << "item record: set pitem to " << pitem << " in connector at pvt " << prec->dpvt << std::endl;
        } catch(std::exception& e) {
            std::cerr << prec->name << " Error in init_record : " << e.what() << std::endl;
            return S_dbLib_badLink;
        }
    }

    return 0;
}

long
process (dbCommon *pdbc)
{
    opcuaItemRecord *prec = reinterpret_cast<opcuaItemRecord *>(pdbc);
    int pact = prec->pact;
    long status = 0;

    status = readValue(prec); /* read the new value */
    if (!pact && prec->pact)
        return 0;

    prec->pact = TRUE;
    recGblGetTimeStamp(prec);

    monitor(prec);

    /* Wrap up */
    recGblFwdLink(prec);
    prec->pact = FALSE;
    return status;
}

long
special (DBADDR *paddr, int after)
{
    opcuaItemRecord *prec = reinterpret_cast<opcuaItemRecord *>(paddr->precord);

    if (!after)
        return 0;

    (void) prec;
    return 0;
}

void
monitor (opcuaItemRecord *prec)
{
    epicsUInt16 events = recGblResetAlarms(prec);

    if (events)
        db_post_events(prec, prec->val, events);
}

long
readValue (opcuaItemRecord *prec)
{
    long status = 0;

    if (prec->pact)
        goto read;

    status = dbGetLink(&prec->siml, DBR_USHORT, &prec->simm, nullptr, nullptr);
    if (status)
        return status;

    switch (prec->simm) {
    case menuYesNoNO:
read:
//        status = pdset->read_string(prec);
        break;

    case menuYesNoYES:
        recGblSetSevr(prec, SIMM_ALARM, prec->sims);
//        status = dbGetLinkLS(&prec->siol, prec->val, prec->sizv, &prec->len);
        break;

    default:
        recGblSetSevr(prec, SOFT_ALARM, INVALID_ALARM);
        status = -1;
    }

    if (!status)
        prec->udf = FALSE;

    return status;
}

/* Create Record Support Entry Table */

#define report nullptr
#define initialize nullptr
/* init_record */
/* process */
/* special */
#define get_value nullptr
#define cvt_dbaddr nullptr
#define get_array_info nullptr
#define put_array_info nullptr
#define get_units nullptr
#define get_precision nullptr
#define get_enum_str nullptr
#define get_enum_strs nullptr
#define put_enum_str nullptr
#define get_graphic_double nullptr
#define get_control_double nullptr
#define get_alarm_double nullptr

rset opcuaItemRSET = {
    RSETNUMBER,
    report,
    initialize,
    RECSUPFUN_CAST(init_record),
    RECSUPFUN_CAST(process),
    RECSUPFUN_CAST(special),
    get_value,
    cvt_dbaddr,
    get_array_info,
    put_array_info,
    get_units,
    get_precision,
    get_enum_str,
    get_enum_strs,
    put_enum_str,
    get_graphic_double,
    get_control_double,
    get_alarm_double
};
extern "C" { epicsExportAddress(rset, opcuaItemRSET); }

} // namespace
