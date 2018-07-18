/*************************************************************************\
* Copyright (c) 2018 ITER Organization.
* This module is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 *  Author: Ralph Lange <ralph.lange@gmx.de>
 *
 *  based on prototype work by Bernhard Kuner <bernhard.kuner@helmholtz-berlin.de>
 *  and example code from the Unified Automation C++ Based OPC UA Client SDK
 */

#include <iostream>
#include <string>
#include <map>

#include <uaclientsdk.h>
#include <uasession.h>

#include <errlog.h>
#include <epicsExport.h>

#include "SubscriptionUaSdk.h"
#include "ItemUaSdk.h"

namespace DevOpcua {

using namespace UaClientSdk;

std::map<std::string, SubscriptionUaSdk*> SubscriptionUaSdk::subscriptions;

SubscriptionUaSdk::SubscriptionUaSdk (const std::string &name, SessionUaSdk *session,
                                      const double publishingInterval, const epicsUInt8 priority,
                                      const int debug)
    : Subscription(debug)
    , name(name)
    , puasubscription(NULL)
    , psessionuasdk(session)
    //TODO: add runtime support for subscription enable/disable
    , enable(true)
{
    // keep the default timeout
    double deftimeout = subscriptionSettings.publishingInterval * subscriptionSettings.lifetimeCount;
    subscriptionSettings.publishingInterval = publishingInterval;
    subscriptionSettings.lifetimeCount = static_cast<OpcUa_UInt32>(deftimeout / publishingInterval);
    subscriptionSettings.priority = priority;

    subscriptions[name] = this;
    psessionuasdk->subscriptions[name] = this;
}

void
SubscriptionUaSdk::show (int level) const
{
    std::cout << "subscription=" << name
              << " session="     << psessionuasdk->getName()
              << " interval=";
    if (puasubscription)
        std::cout << puasubscription->publishingInterval();
    else
        std::cout << "?";
    std::cout << "(" << subscriptionSettings.publishingInterval << ")"
              << " prio=";
    if (puasubscription)
        std::cout << static_cast<int>(puasubscription->priority());
    else
        std::cout << "?";
    std::cout << "(" << static_cast<int>(subscriptionSettings.priority) << ")"
              << " enable=" << (puasubscription ? (puasubscription->publishingEnabled() ? "Y" : "N") : "?")
              << "(" << (enable ? "Y" : "N") << ")"
              << " debug="       << debug
              << std::endl;

    if (level >= 1) {
        for (auto &it : items) {
            it->show(level-1);
        }
    }
}

SubscriptionUaSdk &
SubscriptionUaSdk::findSubscription (const std::string &name)
{
    auto it = subscriptions.find(name);
    if (it == subscriptions.end()) {
        throw std::runtime_error("no such subscription");
    }
    return *(it->second);
}

bool
SubscriptionUaSdk::subscriptionExists (const std::string &name)
{
    auto it = subscriptions.find(name);
    return !(it == subscriptions.end());
}

//TODO: move Subscription::findSubscription() and Subscription::subscriptionExists()
// back to Subscription.cpp after adding implementation management there
Subscription &
Subscription::findSubscription (const std::string &name)
{
    return static_cast<Subscription &>(SubscriptionUaSdk::findSubscription(name));
}

bool
Subscription::subscriptionExists (const std::string &name)
{
    return SubscriptionUaSdk::subscriptionExists(name);
}

void
SubscriptionUaSdk::showAll (int level)
{
    std::cout << "OPC UA: "
              << subscriptions.size() << " subscription(s) configured"
              << std::endl;
    if (level >= 1) {
        for (auto &it : subscriptions) {
            it.second->show(level-1);
        }
    }
}

Session &
SubscriptionUaSdk::getSession() const
{
    return static_cast<Session &>(*psessionuasdk);
}


SessionUaSdk &
SubscriptionUaSdk::getSessionUaSdk() const
{
    return *psessionuasdk;
}

void
SubscriptionUaSdk::create ()
{
    UaStatus status;
    ServiceSettings serviceSettings;

    status = psessionuasdk->puasession->createSubscription(
                serviceSettings,
                this,
                0,
                subscriptionSettings,
                enable,
                &puasubscription);

    if (status.isBad()) {
        errlogPrintf("OPC UA subscription %s: createSubscription on session %s failed (%s)\n",
                     name.c_str(), psessionuasdk->getName().c_str(), status.toString().toUtf8());
    }
}

void
SubscriptionUaSdk::clear ()
{
    puasubscription = NULL;
}

void
SubscriptionUaSdk::addItemUaSdk(ItemUaSdk *item)
{
    items.push_back(item);
}

void
SubscriptionUaSdk::removeItemUaSdk(ItemUaSdk *item)
{
    auto it = std::find(items.begin(), items.end(), item);
    if (it != items.end())
        items.erase(it);
}


// UaSubscriptionCallback interface

void
SubscriptionUaSdk::subscriptionStatusChanged (OpcUa_UInt32 clientSubscriptionHandle,
                                              const UaStatus& status)
{}

void
SubscriptionUaSdk::dataChange (OpcUa_UInt32 clientSubscriptionHandle,
                               const UaDataNotifications& dataNotifications,
                               const UaDiagnosticInfos&   diagnosticInfos)
{}

void
SubscriptionUaSdk::newEvents (OpcUa_UInt32  clientSubscriptionHandle,
                              UaEventFieldLists&          eventFieldList)
{}

} // namespace DevOpcua