/* Copyright Hewlett Packard Enterprise Development LP. */

/**
 * @file ltf_moyos.h
 * @brief Contains the definition of `MoyoImpl` - a moyo service to interact with the WriteBuffer.
 */
#ifndef LTF_MOYOS_H
#define LTF_MOYOS_H

#include "pf_sub_ctrl.h"
#include "u_grpc.h"
#include "clustercfg_sub_ctrl_vector.h"
#include "memcache_node.h"
#include "wb_dsp_impl.h"
#include "wb_object.h"
#include "wb_moyos.pb.h"
#include "wb_moyos.grpc.pb.h"
#include "wb_sub_ctrl_vector.h"
#include "wb_transaction_impl.h"

using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

using ::moyo_server::MoyoStatus;

namespace ltf {

class Superblock;
class SuperblockManager;

/**
 * @class MoyoImpl
 *
 * @brief Implementation of WB moyo service.
 */
class MoyoImpl final : public Svc::Service {
public: /* Constructor */
    MoyoImpl()
        : _generation(0),
          _dsp_internal_id(0)
    {}

public: /* RPC calls */
   
    /**
     * @brief Enable LTF trace record
     *
     * @param[in] context   Server context object
     * @param[in] request   Request parameters for the moyo
     * @param[out] response Status of the enable request
     * @return Status of the RPC call
     */
    Status ltfTraceRecordEnable(ServerContext* context, 
                                const LtfTraceRecordEnableRequest* request, 
                                LtfTraceRecordEnableResponse* response) override;

   
public: /* Moyo permittivity functions */
    /**
     * @brief Is the moyo permitted?
     *
     * @param[in] moyo    The moyo name in string format.
     * @param[in] status  The status, filled with error response if not permitted.
     * @param[in] dsp_id  The DSP internal id for which the moyo is to be run.
     *
     * @return true if SoS mode is disabled OR the moyo is allowed, false otherwise.
     */
    bool is_moyo_permitted(const char* moyo,
                           MoyoStatus* status,
                           obj::DspInternalId dsp_id) const;

    /**
     * @brief Get submodule mode
     *
     * @param[in] dsp_internal_id  The DSP internal id for which the mode is to be returned.
     */
    PfMode get_submodule_mode(obj::DspInternalId dsp_internal_id) const;

private: /* Miscellaneous Moyo helper functions */
    /**
     * @brief Checks whether the current NVO type is present in the
     *        list of requested NVO types. This is for use in verifyNvos.
     *
     * @param[in] nvo          The NVO.
     * @param[in] nvo_types    The list of request NVO types.
     * @return Presence of the NVO type in the list of NVO types.
     */
    bool _is_nvotype_present(const NvObjectBase& nvo,
                             const std::vector<string>& nvo_types) const;

private: /* Data members */
    /** @brief Map to maintain DSP id to TransactionIdMap. */
    typedef std::map<const obj::DspId, TransactionIdMap*> TransactionDspIdMap;

    /** @brief Iterator for TransactionMap. */
    typedef TransactionDspIdMap::iterator TransactionDspIdMapIterator;

    /**
     * @brief Map to maintain transaction id to transaction impl instance pointer, per DSP id.
     */
    TransactionDspIdMap _transaction_map;

    /** @brief Map to maintain NVO id to NvObjectBase instance pointer. */
    typedef std::map<NvObjectId, NvObjectBase*> NvoIdMap;

    /** @brief Iterator for NvoIdMap. */
    typedef NvoIdMap::iterator NvoIdMapIterator;

    /** @brief Map to maintain DSP id to NvObjectBase instance pointer. */
    typedef std::map<const obj::DspId, NvoIdMap*> NvoDspIdMap;

    /** @brief Iterator for NvoMap. */
    typedef NvoDspIdMap::iterator NvoDspIdMapIterator;

    /**
     * @brief Map to maintain Nvo id to NvObjectBase instance pointer, per DSP id
     */
    NvoDspIdMap _nvo_map;

    /**
     * @brief Each time the transaction map or NVO map is modified _generation
     *        will be incremented.
     */
    std::atomic<uint64_t> _generation;

    /**
     * @brief Useful while defining PfSubComponentId during writebuffer
     *        DSP creation.
     */
    std::atomic<obj::DspInternalId> _dsp_internal_id;
}; /* class MoyoImpl */

} /* namespace ltf */

#endif /* LTF_MOYOS_H */

