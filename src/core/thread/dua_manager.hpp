/*
 *  Copyright (c) 2020, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes definitions for managing Domain Unicast Address feature defined in Thread 1.2.
 */

#ifndef DUA_MANAGER_HPP_
#define DUA_MANAGER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_DUA_ENABLE || OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE

#include "backbone_router/bbr_leader.hpp"
#include "coap/coap.hpp"
#include "coap/coap_message.hpp"
#include "common/locator.hpp"
#include "common/notifier.hpp"
#include "common/tasklet.hpp"
#include "common/time.hpp"
#include "common/timer.hpp"
#include "net/netif.hpp"
#include "thread/thread_tlvs.hpp"
#include "thread/topology.hpp"

namespace ot {

/**
 * @addtogroup core-dua
 *
 * @brief
 *   This module includes definitions for generating, managing, registering Domain Unicast Address.
 *
 * @{
 *
 * @defgroup core-dua Dua
 *
 * @}
 *
 */

/**
 * This class implements managing DUA.
 *
 */
class DuaManager : public InstanceLocator
{
    friend class ot::Notifier;

public:
    /**
     * This constructor initializes the object.
     *
     * @param[in]  aInstance     A reference to the OpenThread instance.
     *
     */
    explicit DuaManager(Instance &aInstance);

    /**
     * This method notifies Domain Prefix status.
     *
     * @param[in]  aState  The Domain Prefix state or state change.
     *
     */
    void HandleDomainPrefixUpdate(BackboneRouter::Leader::DomainPrefixState aState);

    /**
     * This method notifies Primary Backbone Router status.
     *
     * @param[in]  aState   The state or state change of Primary Backbone Router.
     * @param[in]  aConfig  The Primary Backbone Router service.
     *
     */
    void HandleBackboneRouterPrimaryUpdate(BackboneRouter::Leader::State               aState,
                                           const BackboneRouter::BackboneRouterConfig &aConfig);

#if OPENTHREAD_CONFIG_DUA_ENABLE

    /**
     * This method returns a reference to the Domain Unicast Address.
     *
     * @returns A reference to the Domain Unicast Address.
     *
     */
    const Ip6::Address &GetDomainUnicastAddress(void) const { return mDomainUnicastAddress.GetAddress(); }

    /**
     * This method sets the Interface Identifier manually specified for the Thread Domain Unicast Address.
     *
     * @param[in]  aIid        A reference to the Interface Identifier to set.
     *
     * @retval OT_ERROR_NONE           Successfully set the Interface Identifier.
     * @retval OT_ERROR_INVALID_ARGS   The specified Interface Identifier is reserved.
     *
     */
    otError SetFixedDuaInterfaceIdentifier(const Ip6::InterfaceIdentifier &aIid);

    /**
     * This method clears the Interface Identifier manually specified for the Thread Domain Unicast Address.
     *
     */
    void ClearFixedDuaInterfaceIdentifier(void);

    /**
     * This method indicates whether or not there is Interface Identifier manually specified for the Thread
     * Domain Unicast Address.
     *
     * @retval true  If there is Interface Identifier manually specified.
     * @retval false If there is no Interface Identifier manually specified.
     *
     */
    bool IsFixedDuaInterfaceIdentifierSet(void) { return !mFixedDuaInterfaceIdentifier.IsUnspecified(); }

    /**
     * This method gets the Interface Identifier for the Thread Domain Unicast Address if manually specified.
     *
     * @returns A reference to the Interface Identifier.
     *
     */
    const Ip6::InterfaceIdentifier &GetFixedDuaInterfaceIdentifier(void) const { return mFixedDuaInterfaceIdentifier; }

    /*
     * This method restores duplicate address detection information from non-volatile memory.
     *
     */
    void Restore(void);
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    void UpdateChildDomainUnicastAddress(const Child &aChild, Mle::ChildDuaState aState);
#endif

private:
    enum
    {
        kNewRouterRegistrationDelay = 5,    ///< Delay (in seconds) for waiting link establishment for a new Router.
        kStateUpdatePeriod          = 1000, ///< 1000ms period  (i.e. 1s)
    };

#if OPENTHREAD_CONFIG_DUA_ENABLE
    otError GenerateDomainUnicastAddressIid(void);
    otError Store(void);

    void AddDomainUnicastAddress(void);
    void RemoveDomainUnicastAddress(void);
    void UpdateRegistrationDelay(uint8_t aDelay);
#endif

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    void SendAddressNotification(Ip6::Address &aAddress, ThreadStatusTlv::DuaStatus aStatus, const Child &aChild);
#endif

    void HandleNotifierEvents(Events aEvents);

    static void HandleTimer(Timer &aTimer) { aTimer.GetOwner<DuaManager>().HandleTimer(); }

    void HandleTimer(void);

    static void HandleRegistrationTask(Tasklet &aTasklet) { aTasklet.GetOwner<DuaManager>().PerformNextRegistration(); }

    void ScheduleTimer(void);

    static void HandleDuaResponse(void *               aContext,
                                  otMessage *          aMessage,
                                  const otMessageInfo *aMessageInfo,
                                  otError              aResult)
    {
        static_cast<DuaManager *>(aContext)->HandleDuaResponse(
            *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo), aResult);
    }

    void HandleDuaResponse(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo, otError aResult);

    static void HandleDuaNotification(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
    {
        static_cast<DuaManager *>(aContext)->HandleDuaNotification(
            *static_cast<Coap::Message *>(aMessage), *static_cast<const Ip6::MessageInfo *>(aMessageInfo));
    }

    void    HandleDuaNotification(Coap::Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    otError ProcessDuaResponse(Coap::Message &aMessage);

    void PerformNextRegistration(void);
    void UpdateReregistrationDelay(void);
    void UpdateCheckDelay(uint8_t aDelay);

    TimerMilli     mTimer;
    Tasklet        mRegistrationTask;
    Coap::Resource mDuaNotification;

    bool mIsDuaPending : 1;

#if OPENTHREAD_CONFIG_DUA_ENABLE
    enum DuaState
    {
        kNotExist,    ///< DUA is not avaiable.
        kToRegister,  ///< DUA is to be registered.
        kRegistering, ///< DUA is being registered.
        kRegistered,  ///< DUA is registered.
    };

    DuaState  mDuaState : 2;
    uint8_t   mDadCounter;
    TimeMilli mLastRegistrationTime; ///< The time (in milliseconds) when sent last DUA.req or received DUA.rsp.
    Ip6::InterfaceIdentifier mFixedDuaInterfaceIdentifier;
    Ip6::NetifUnicastAddress mDomainUnicastAddress;
#endif

    union
    {
        struct
        {
            uint16_t mReregistrationDelay; ///< Delay (in seconds) for DUA re-registration.
            uint8_t  mCheckDelay;          ///< Delay (in seconds) for checking whether or not registration is required.
#if OPENTHREAD_CONFIG_DUA_ENABLE
            uint8_t mRegistrationDelay; ///< Delay (in seconds) for DUA registration.
#endif
        } mFields;
        uint32_t mValue; ///< Non-zero indicates timer should start.
    } mDelay;

#if OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
    // TODO: (DUA) may re-evaluate the alternative option of distributing the flags into the child table:
    //       - Child class itself have some padding - may save some RAM
    //       - Avoid cross reference between a bit-vector and the child entry
    ChildMask mChildDuaMask;           ///< Child Mask for child who registers DUA via Child Update Request.
    ChildMask mChildDuaRegisteredMask; ///< Child Mask for child's DUA that was registered by the parent on behalf.
    uint16_t  mChildIndexDuaRegistering : 15; ///< Child Index of the DUA being registered.
    bool      mRegisterCurrentChildIndex : 1; ///< Re-register the child just registered.
#endif
};

} // namespace ot

#endif // OPENTHREAD_CONFIG_DUA_ENABLE || OPENTHREAD_CONFIG_TMF_PROXY_DUA_ENABLE
#endif // DUA_MANAGER_HPP_