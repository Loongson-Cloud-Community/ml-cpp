/*
 * Copyright Elasticsearch B.V. and/or licensed to Elasticsearch B.V. under one
 * or more contributor license agreements. Licensed under the Elastic License
 * 2.0 and the following additional limitation. Functionality enabled by the
 * files subject to the Elastic License 2.0 may only be used in production when
 * invoked by an Elasticsearch process with a license key installed that permits
 * use of machine learning features. You may not use this file except in
 * compliance with the Elastic License 2.0 and the foregoing additional
 * limitation.
 */

#ifndef INCLUDED_ml_api_CDataFrameAnalysisInstrumentation_h
#define INCLUDED_ml_api_CDataFrameAnalysisInstrumentation_h

#include <core/CProgramCounters.h>
#include <core/CRapidJsonConcurrentLineWriter.h>

#include <maths/analytics/CDataFrameAnalysisInstrumentationInterface.h>
#include <maths/common/CBasicStatistics.h>

#include <api/ApiTypes.h>
#include <api/ImportExport.h>

#include <rapidjson/document.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

namespace ml {
namespace api {

//! \brief Instrumentation class for collecting data frame analysis job statistics.
//!
//! DESCRIPTION:\n
//! Responsible for collecting data frame analysis job statistics, i.e. memory usage,
//! progress, parameters, quality of results. This also implements the functionality
//! to write the JSON statistics to a specified output stream in a thread safe manner.
//!
//! IMPLEMENTATION DECISIONS:\n
//! With the exception of reading and writing progress and memory usage this class is
//! *NOT* thread safe. It is expected that calls to update and write instrumentation
//! data all happen on the thread running the analysis. It also performs thread safe
//! writing to a shared output stream. For example, it is expected that writes for
//! progress happen concurrently with writes of other instrumentation.
class API_EXPORT CDataFrameAnalysisInstrumentation
    : virtual public maths::analytics::CDataFrameAnalysisInstrumentationInterface {
public:
    //!\brief Memory status
    enum EMemoryStatus { E_Ok, E_HardLimit };

    //! \brief Set the output stream for the lifetime of this object.
    class API_EXPORT CScopeSetOutputStream {
    public:
        CScopeSetOutputStream(CDataFrameAnalysisInstrumentation& instrumentation,
                              core::CJsonOutputStreamWrapper& outStream);
        ~CScopeSetOutputStream();

        CScopeSetOutputStream(const CScopeSetOutputStream&) = delete;
        CScopeSetOutputStream& operator=(const CScopeSetOutputStream&) = delete;

    private:
        CDataFrameAnalysisInstrumentation& m_Instrumentation;
    };

public:
    //! Constructs an instrumentation object an analytics job with a given \p jobId.
    CDataFrameAnalysisInstrumentation(const std::string& jobId, std::size_t memoryLimit);

    //! Adds \p delta to the memory usage statistics.
    void updateMemoryUsage(std::int64_t delta) override;

    //! Start progress monitoring for \p phase.
    //!
    //! \note This resets the current progress to zero.
    void startNewProgressMonitoredTask(const std::string& task) override;

    //! This adds \p fractionalProgress to the current progress.
    //!
    //! \note The caller should try to ensure that the sum of the values added
    //! at the end of the analysis is equal to one.
    //! \note This is converted to an integer - so we can atomically add - by
    //! scaling by 1024. Therefore, this shouldn't be called with values less
    //! than 0.001. In fact, it is unlikely that such high resolution is needed
    //! and typically this would be called significantly less frequently.
    void updateProgress(double fractionalProgress) override;

    //! Reset variables related to the job progress.
    void resetProgress();

    //! Record that the analysis is complete.
    void setToFinished();

    //! \return True if the running analysis has finished.
    bool finished() const;

    //! \return The progress of the analysis in the range [0,1] being an estimate
    //! of the proportion of total work complete for a single run.
    double progress() const;

    //! Flush then reinitialize the instrumentation data. This will trigger
    //! writing them to the results pipe.
    void flush(const std::string& tag = "") override;

    //! \return The peak memory usage.
    std::int64_t memory() const;

    //! \return The id of the data frame analytics job.
    const std::string& jobId() const;

    //! Start polling and writing progress updates.
    //!
    //! \note This doesn't return until instrumentation.setToFinished() is called.
    static void monitor(CDataFrameAnalysisInstrumentation& instrumentation,
                        core::CRapidJsonConcurrentLineWriter& writer);

protected:
    using TWriter = core::CRapidJsonConcurrentLineWriter;
    using TWriterUPtr = std::unique_ptr<TWriter>;
    using TOptionalInt64 = std::optional<std::int64_t>;

protected:
    virtual counter_t::ECounterTypes memoryCounterType() = 0;
    TWriter* writer();
    void memoryReestimate(std::int64_t memoryReestimate);
    void memoryStatus(EMemoryStatus status);

private:
    static const std::string NO_TASK;

private:
    std::string readProgressMonitoredTask() const;
    int percentageProgress() const;
    virtual void writeAnalysisStats(std::int64_t timestamp) = 0;
    void writeMemoryAndAnalysisStats();
    void writeMemory(std::int64_t timestamp);
    static void writeProgress(const std::string& task,
                              int progress,
                              core::CRapidJsonConcurrentLineWriter* writer);

private:
    std::string m_JobId;
    std::string m_ProgressMonitoredTask;
    std::int64_t m_MemoryLimit;
    std::atomic_bool m_Finished;
    std::atomic_size_t m_FractionalProgress;
    std::atomic<std::int64_t> m_Memory;
    mutable std::mutex m_ProgressMutex;
    TWriterUPtr m_Writer;
    EMemoryStatus m_MemoryStatus;
    TOptionalInt64 m_MemoryReestimate;
};

//! \brief Instrumentation class for Outlier Detection jobs.
class API_EXPORT CDataFrameOutliersInstrumentation final
    : public CDataFrameAnalysisInstrumentation,
      public maths::analytics::CDataFrameOutliersInstrumentationInterface {
public:
    CDataFrameOutliersInstrumentation(const std::string& jobId, std::size_t memoryLimit)
        : CDataFrameAnalysisInstrumentation(jobId, memoryLimit) {}
    void parameters(const maths::analytics::COutliers::SComputeParameters& parameters) override;
    void elapsedTime(std::uint64_t time) override;
    void featureInfluenceThreshold(double featureInfluenceThreshold) override;

protected:
    counter_t::ECounterTypes memoryCounterType() override;

private:
    void writeAnalysisStats(std::int64_t timestamp) override;
    void writeTimingStats(rapidjson::Value& parentObject);
    void writeParameters(rapidjson::Value& parentObject);

private:
    maths::analytics::COutliers::SComputeParameters m_Parameters;
    std::uint64_t m_ElapsedTime;
    double m_FeatureInfluenceThreshold = -1.0;
    bool m_AnalysisStatsInitialized = false;
};

//! \brief Instrumentation class for Supervised Learning jobs.
//!
//! DESCRIPTION:\n
//! This class extends CDataFrameAnalysisInstrumentation with setters
//! for hyperparameters, validation loss results, and job timing.
class API_EXPORT CDataFrameTrainBoostedTreeInstrumentation final
    : public CDataFrameAnalysisInstrumentation,
      public maths::analytics::CDataFrameTrainBoostedTreeInstrumentationInterface {
public:
    CDataFrameTrainBoostedTreeInstrumentation(const std::string& jobId, std::size_t memoryLimit)
        : CDataFrameAnalysisInstrumentation(jobId, memoryLimit) {}

    //! Set the supervised learning job \p type, can be E_Regression or E_Classification.
    void type(EStatsType type) override;
    //! Set the current \p iteration number.
    void iteration(std::size_t iteration) override;
    //! Set the run time of the current iteration.
    void iterationTime(std::uint64_t delta) override;
    //! Set the type of the validation loss result, e.g. "mse".
    void lossType(const std::string& lossType) override;
    //! Set the validation loss values for \p fold for each forest size to \p lossValues.
    void lossValues(std::size_t fold, TDoubleVec&& lossValues) override;
    //! Set the analysis task.
    void task(api_t::EDataFrameTrainBoostedTreeTask task);
    //! \return A writable object containing the training hyperparameters.
    SHyperparameters& hyperparameters() override { return m_Hyperparameters; }

protected:
    counter_t::ECounterTypes memoryCounterType() override;

private:
    using TLossVec = std::vector<std::pair<std::size_t, TDoubleVec>>;
    using TRowsAccumulator =
        maths::common::CBasicStatistics::SSampleMean<std::uint32_t>::TAccumulator;

private:
    void writeAnalysisStats(std::int64_t timestamp) override;
    void writeHyperparameters(rapidjson::Value& parentObject);
    void writeValidationLoss(rapidjson::Value& parentObject);
    void writeTimingStats(rapidjson::Value& parentObject);
    void reset();

private:
    EStatsType m_Type{E_Regression};
    std::size_t m_Iteration{0};
    std::uint64_t m_IterationTime{0};
    std::uint64_t m_ElapsedTime{0};
    bool m_AnalysisStatsInitialized{false};
    std::string m_LossType;
    TLossVec m_LossValues;
    api_t::EDataFrameTrainBoostedTreeTask m_Task{api_t::E_Train};
    SHyperparameters m_Hyperparameters;
};
}
}

#endif // INCLUDED_ml_api_CDataFrameAnalysisInstrumentation_h
