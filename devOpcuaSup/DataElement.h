/*************************************************************************\
* Copyright (c) 2018 ITER Organization.
* This module is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 *  Author: Ralph Lange <ralph.lange@gmx.de>
 *
 *  based on prototype work by Bernhard Kuner <bernhard.kuner@helmholtz-berlin.de>
 */

#ifndef DEVOPCUA_DATAELEMENT_H
#define DEVOPCUA_DATAELEMENT_H

#include <vector>
#include <memory>

#include <epicsTypes.h>
#include <epicsTime.h>

#include "devOpcua.h"

namespace DevOpcua {

class RecordConnector;

/**
 * @brief The DataElement interface for a single piece of data.
 *
 * A data element can either be the top level data of an item (in that case
 * its name is an empty string) or be an element of a structured data type (in
 * that case name is the data element name).
 *
 * Inside a structure, a data element can either be a leaf, i.e. be of one of the
 * builtin types and connected to a record (through the pconnector member) or be
 * a node of a structured data type and contain a list of its child elements.
 *
 * As resource conflicts can only occur in nodes that are accessed by records
 * (database side) and items (OPC UA side), the RecordConnector lock must be held
 * when operating on a data element.
 */
class DataElement
{
public:
    virtual ~DataElement();

    /**
     * @brief Setter to create a (bidirectional) link to a RecordConnector.
     *
     * Sets the internal pointer to the record connector as well as the
     * inverse link in the record connector that points back.
     *
     * An existing link is cleanly removed before the new link is set up.
     *
     * @param connector  pointer to the RecordConnector to link to
     */
    void setRecordConnector(RecordConnector *connector);

    /**
     * @brief Read the time stamp of the incoming data.
     *
     * The server flag selects the time stamp to read:
     * true = read server time stamp
     * false = device time stamp
     *
     * @param server  select server time stamp
     * @return EPICS time stamp
     */
    virtual epicsTimeStamp readTimeStamp(bool server = true) const = 0;

    /**
     * @brief Read incoming data as Int32.
     *
     * @return value as epicsInt32
     *
     * @throws std::runtime_error if no data present or on conversion error
     */
    virtual epicsInt32 readInt32() const = 0;

    /**
     * @brief Read incoming data as UInt32.
     *
     * @return value as epicsUInt32
     *
     * @throws std::runtime_error if no data present or on conversion error
     */
    virtual epicsUInt32 readUInt32() const = 0;

    /**
     * @brief Read incoming data as Float64.
     *
     * @return value as epicsFloat64
     *
     * @throws std::runtime_error if no data present or on conversion error
     */
    virtual epicsFloat64 readFloat64() const = 0;
//    virtual epicsOldString readOldString() const = 0;

    /**
     * @brief Check status of last read service.
     *
     * @return true = last read service ok
     */
    virtual bool readWasOk() const = 0;

    /**
     * @brief Write outgoing Int32 data.
     *
     * @param value  value to write
     *
     * @throws std::runtime_error on conversion error
     */
    virtual void writeInt32(const epicsInt32 &value) = 0;

    /**
     * @brief Check status of last write service.
     *
     * @return true = last write service ok
     */
    virtual bool writeWasOk() const = 0;

    /**
     * @brief Clear (discard) the current incoming data.
     *
     * Called by the device support (still holding the RecordConnector lock!)
     * after it is done accessing the data in the context of processing.
     *
     * In case an implementation uses a queue, this should remove the
     * current (= oldest) element from the queue, allowing access to the
     * next element with the next processing.
     */
    virtual void clearIncomingData() = 0;

    /**
     * @brief Create processing requests for record(s) attached to this element.
     */
    virtual void requestRecordProcessing(const ProcessReason reason) const = 0;

protected:
    /**
     * @brief Constructor for DataElement, to be used by implementations.
     *
     * @param name  structure element name (empty otherwise)
     */
    DataElement(const std::string &name = "");

    std::string name;                                    /**< element name */
    std::vector<std::unique_ptr<DataElement>> elements;  /**< children (if node) */
    RecordConnector *pconnector;                         /**< connector (if leaf) */
};

} // namespace DevOpcua

#endif // DEVOPCUA_DATAELEMENT_H
