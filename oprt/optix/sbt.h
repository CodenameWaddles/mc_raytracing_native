#pragma once

#include <optix.h>
#include <concepts>
#include <type_traits>
#include <oprt/core/cudabuffer.h>

namespace oprt {

/**
 * @note 
 * Should I implement a wrapper for Optix Shader Binding Table ??
 */

namespace {
    template <class T>
    concept TrivialCopyable = std::is_trivially_copyable_v<T>;

    template <class Head, class... Args>
    void push_to_vector(std::vector<Head>& v, const Head& head, const Args&... args)
    {
        v.emplace_back(head);
        if constexpr (sizeof...(args) != 0)
            push_to_vector(v, args...);
    }
} // ::nonamed namespace

#ifndef __CUDACC__

/**
 * @note 
 * 各種データが仮想関数を保持したしている場合、デバイス上におけるインスタンス周りで 
 * Invalid accessが生じる可能性があるので <concepts> 等で制約をかけたい
 * 
 * SPECIALTHANKS:
 * @yaito3014 gave me an advise that to use is_trivially_copyable_v 
 * for prohibiting declaration of virtual functions in SBT Data.
 */

template <TrivialCopyable T>
struct Record 
{
    __align__ (OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;
};

template <class RayGenRecord, class MissRecord, class HitGroupRecord, 
          class CallablesRecord, class ExceptionRecord, size_t NRay>
class ShaderBindingTable {
public:
    ShaderBindingTable()
    {
        static_assert(NRay > 0, "The number of ray types must be at least 1.");
    }

    explicit operator OptixShaderBindingTable() const { return m_sbt; }

    void setRayGenRecord(const RayGenRecord& rg_record)
    {
        m_raygen_record = rg_record;
    }

    template <class... MissRecordArgs>
    void setMissRecord(const MissRecordArgs&... args)
    {
        static_assert(sizeof...(args) == N, 
            "oprt::ShaderBindingTable::addMissRecord(): The number of record must be same with the number of ray types.");        
        static_assert(std::conjunction<std::is_same<MissRecord, Args>...>::value, 
            "oprt::ShaderBindingTable::addMissRecord(): Data type must be same with 'MissRecord'.");

        int j = 0;
        for (auto i : std::initializer_list<std::common_type_t<MissRecordArgs...>>{args...})
            m_miss_records[j++] = i;
    }

    /// @note 置き換えを行ったらデバイス上のデータも更新する？
    void replaceMissRecord(const MissRecord& record, const int idx)
    {
        if (idx >= m_miss_records.size())
        {
            Message(MSG_ERROR, "oprt::ShaderBindingTable::replaceMissRecord(): The index out of range.");
            return;
        }
        m_miss_records[idx] = record;
    }

    MissRecord getMissRecord(const int idx) const {
        m_miss_records[idx] = record;
    }

    template <class... HitGroupRecordArgs> 
    void addHitGroupRecord(const HitGroupRecordArgs...& args)
    {
        static_assert(sizeof...(args) == N, 
            "oprt::ShaderBindingTable::addHitgroupRecord(): The number of hitgroup record must be same with the number of ray types.");        
        static_assert(std::conjunction<std::is_same<HitGroupRecord, Args>...>::value, 
            "oprt::ShaderBindingTable::addHitgroupRecord(): Record type must be same with 'HitGroupRecord'.");
        push_to_vector(m_hitgroup_records, args...);
    }

    /// @note 置き換えを行ったらデバイス上のデータも更新する？
    void replaceHitGroupRecord(const HitGroupRecord& record, const int idx)
    {
        if (idx >= m_hitgroup_records.size())
        {
            Message(MSG_ERROR, "oprt::ShaderBindingTable::replaceHitGroupRecord(): The index out of range.");
            return;
        }
        m_hitgroup_records[idx] = record;
    }

    template <class... CallablesRecordArgs>
    void addCallablesRecord(const CallablesRecordArgs&... args)
    {
        static_assert(std::conjunction<std::is_same<CallablesRecord, Args>...>::value, 
            "oprt::ShaderBindingTable::addCallablesRecord(): Record type must be same with 'CallablesRecord'.");

        push_to_vector(m_callables_records, args...);
    }

    /// @note 置き換えを行ったらデバイス上のデータも更新する？
    void replaceCallablesRecord(const CallablesRecord& record, const int idx)
    {
        if (idx >= m_callables_records.size())
        {
            Message(MSG_ERROR, "oprt::ShaderBindingTable::replaceCallablesRecord(): The index out of range.");
            return;
        }
        m_callables_records[idx] = record;
    }

    void setExceptionRecord(const ExceptionRecord& ex_record)
    {
        m_exception_record = ex_record;
    }

    void createOnDevice()
    {
        CUDABuffer<RayGenRecord> d_raygen_record;
        CUDABuffer<MissRecord> d_miss_records;
        CUDABuffer<HitGroupRecord> d_hitgroup_records;
        CUDABuffer<CallablesRecord> d_callables_records;
        CUDABuffer<ExceptionRecord> d_exception_records;

        d_raygen_record.copyToDevice(&m_raygen_record, sizeof(RayGenRecord));
        d_miss_records.copyToDevice(m_miss_record.data(), m_miss_records.size() * sizeof(MissRecord));
        d_hitgroup_records.copyToDevice(m_hitgroup_records.data(), m_hitgroup_records.size() * sizeof(HitGroupRecord));
        d_callables_records.copyToDevice(m_callables_records.data(), m_callables_records.size() * sizeof(CallablesRecord));
        d_exception_records.copyToDevice(m_exception_record.data(), sizeof(ExceptionRecord));

        m_sbt.raygenRecord = d_raygen_record.devicePtr();
        m_sbt.missRecordBase = d_miss_record.devicePtr();
        m_sbt.missRecordCount = static_cast<uint32_t>(m_miss_record.size());
        m_sbt.missRecordStrideInBytes = static_cast<uint32_t>(sizeof(MissRecord));
        m_sbt.hitgroupRecordBase = d_hitgroup_record.devicePtr();
        m_sbt.hitgroupRecordCount = static_cast<uint32_t>(m_hitgroup_records.size());
        m_sbt.hitgroupRecordStrideInBytes = static_cast<uint32_t>(sizeof(HitGroupRecord));
        m_sbt.callablesRecordBase = d_callables_records.devicePtr();
        m_sbt.callablesRecordCount = static_cast<uint32_t>(m_callables_records.size());
        m_sbt.callablesRecordStrideInBytes = static_cast<uint32_t>(sizeof(CallablesRecord));
        m_sbt.raygenRecord = d_raygen_record.devicePtr();

        on_device = true;
    }

    OptixShaderBindingTable sbt() const 
    {
        return m_sbt;
    }

    bool isOnDevice() const 
    {
        return on_device;
    }
private:
    OptixShaderBindingTable m_sbt {};
    RayGenRecord m_raygen_record;
    // std::vector<MissRecord> m_miss_records;
    MissRecord m_miss_records[NRay];
    std::vector<HitGroupRecord> m_hitgroup_records;
    std::vector<CallablesRecord> m_callables_records;
    ExceptionRecord m_exception_record;
    bool on_device;
};

#endif

}