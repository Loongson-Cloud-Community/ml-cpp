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

#ifndef INCLUDED_ml_api_CDataFrameAnalysisSpecification_h
#define INCLUDED_ml_api_CDataFrameAnalysisSpecification_h

#include <core/CDataAdder.h>
#include <core/CDataSearcher.h>
#include <core/CFastMutex.h>
#include <core/CJsonOutputStreamWrapper.h>

#include <api/CDataFrameAnalysisRunner.h>
#include <api/ImportExport.h>

#include <rapidjson/fwd.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace ml {
namespace core {
class CDataFrame;
class CTemporaryDirectory;
}
namespace api {

//! \brief Parses a complete specification for running a core::CDataFrame analysis
//! and supports launching that analysis on a specified frame object.
//!
//! DESCRIPTION:\n
//! This manages extracting all configuration for a particular analysis from a JSON
//! header which is passed to the data_frame_analyzer command before any data. This
//! creates and owns an analysis runner object which is also configured by the header.
//! The analysis is run asynchronously via the CDataFrameAnalysisSpecification::run
//! method which returns a handle to the runner to retrieve progress, errors and other
//! performance statistics.
class API_EXPORT CDataFrameAnalysisSpecification {
public:
    using TBoolVec = std::vector<bool>;
    using TSizeVec = std::vector<std::size_t>;
    using TStrVec = std::vector<std::string>;
    using TDataFrameUPtr = std::unique_ptr<core::CDataFrame>;
    using TTemporaryDirectoryPtr = std::shared_ptr<core::CTemporaryDirectory>;
    using TDataAdderUPtr = std::unique_ptr<ml::core::CDataAdder>;
    using TPersisterSupplier = std::function<TDataAdderUPtr()>;
    using TDataSearcherUPtr = std::unique_ptr<ml::core::CDataSearcher>;
    using TRestoreSearcherSupplier = std::function<TDataSearcherUPtr()>;
    using TDataFrameUPtrTemporaryDirectoryPtrPr =
        std::pair<TDataFrameUPtr, TTemporaryDirectoryPtr>;
    using TRunnerUPtr = std::unique_ptr<CDataFrameAnalysisRunner>;
    using TRunnerFactoryUPtr = std::unique_ptr<CDataFrameAnalysisRunnerFactory>;
    using TRunnerFactoryUPtrVec = std::vector<TRunnerFactoryUPtr>;

public:
    static const std::string JOB_ID;
    static const std::string ROWS;
    static const std::string COLS;
    static const std::string MEMORY_LIMIT;
    static const std::string THREADS;
    static const std::string TEMPORARY_DIRECTORY;
    static const std::string RESULTS_FIELD;
    static const std::string MISSING_FIELD_VALUE;
    static const std::string CATEGORICAL_FIELD_NAMES;
    static const std::string DISK_USAGE_ALLOWED;
    static const std::string ANALYSIS;
    static const std::string NAME;
    static const std::string PARAMETERS;

public:
    //! Initialize from a JSON object.
    //!
    //! The specification has the following expected form:
    //! <CODE>
    //! {
    //!   "job_id": <string>,
    //!   "rows": <integer>,
    //!   "cols": <integer>,
    //!   "memory_limit": <integer>,
    //!   "threads": <integer>,
    //!   "temp_dir": <string>,
    //!   "results_field": <string>,
    //!   "categorical_fields": [<string>],
    //!   "disk_usage_allowed": <boolean>,
    //!   "analysis": {
    //!     "name": <string>,
    //!     "parameters": <object>
    //!   }
    //! }
    //! </CODE>
    //!
    //! \param[in] jsonSpecification The specification as a JSON object.
    //! \note The analysis name must be one of the supported analysis types.
    //! \note All constraints must be positive.
    //! \note The parameters, if any, must be consistent for the analysis type.
    //! \note If this fails the state is set to bad and the analysis will not run.
    //! \note temp_dir Is a directory which can be used to store the data frame
    //! out-of-core if we can't meet the memory constraint for the analysis without
    //! partitioning.
    //! \param persisterSupplier Shared pointer to the CDataAdder instance.
    explicit CDataFrameAnalysisSpecification(
        const std::string& jsonSpecification,
        TDataFrameUPtrTemporaryDirectoryPtrPr* frameAndDirectory = nullptr,
        TPersisterSupplier persisterSupplier = noopPersisterSupplier,
        TRestoreSearcherSupplier restoreSearcherSupplier = noopRestoreSearcherSupplier);

    //! This construtor provides support for custom analysis types and is mainly
    //! intended for testing.
    //!
    //! \param[in] runnerFactories Plugins for the supported analyses.
    CDataFrameAnalysisSpecification(
        TRunnerFactoryUPtrVec runnerFactories,
        const std::string& jsonSpecification,
        TDataFrameUPtrTemporaryDirectoryPtrPr* frameAndDirectory = nullptr,
        TPersisterSupplier persisterSupplier = noopPersisterSupplier,
        TRestoreSearcherSupplier restoreSearcherSupplier = noopRestoreSearcherSupplier);

    CDataFrameAnalysisSpecification(const CDataFrameAnalysisSpecification&) = delete;
    CDataFrameAnalysisSpecification& operator=(const CDataFrameAnalysisSpecification&) = delete;
    CDataFrameAnalysisSpecification(CDataFrameAnalysisSpecification&&) = delete;
    CDataFrameAnalysisSpecification& operator=(CDataFrameAnalysisSpecification&&) = delete;

    //! \return The number of rows in the frame.
    std::size_t numberRows() const;

    //! \return The number of columns in the input frame.
    std::size_t numberColumns() const;

    //! \return The number of columns the analysis configured to run will append
    //! to the data frame.
    std::size_t numberExtraColumns() const;

    //! \return The memory usage limit for the process.
    std::size_t memoryLimit() const;

    //! \return The number of threads the analysis can use.
    std::size_t numberThreads() const;

    //! \return The name of the results field.
    const std::string& resultsField() const;

    //! \return The jobId.
    const std::string& jobId() const;

    //! \return The analysis name.
    const std::string& analysisName() const;

    //! \return The special string signifying a missing value.
    const std::string& missingFieldValue() const;

    //! \return The names of the categorical fields.
    const TStrVec& categoricalFieldNames() const;

    //! \return If it is allowed to overflow data frame to the disk if it doesn't
    //! fit in memory.
    bool diskUsageAllowed() const;

    //! \return The temporary directory if this analysis is using disk storage.
    const std::string& temporaryDirectory() const;

    //! Validate if \p frame is suitable for running the analysis on.
    bool validate(const core::CDataFrame& frame) const;

    //! \return A handle to the object responsible for running the analysis.
    CDataFrameAnalysisRunner* runner();

    //! Estimates memory usage in two cases:
    //!   1. disk is not used (the whole data frame fits in main memory)
    //!   2. disk is used (only one partition needs to be loaded to main memory)
    void estimateMemoryUsage(CMemoryUsageEstimationResultJsonWriter& writer) const;

    //! \return The stream to which to persist state if there is one.
    TDataAdderUPtr persister() const;

    //! \return The stream from which to retore state if there is one.
    TDataSearcherUPtr restoreSearcher() const;

    //! Persister supplier without any action.
    static TDataAdderUPtr noopPersisterSupplier();

    //! Restore search supplier without any action.
    static TDataSearcherUPtr noopRestoreSearcherSupplier();

private:
    void initializeRunner(const rapidjson::Value& jsonAnalysis,
                          TDataFrameUPtrTemporaryDirectoryPtrPr* frameAndDirectory);

private:
    std::size_t m_NumberRows = 0;
    std::size_t m_NumberColumns = 0;
    std::size_t m_MemoryLimit = 0;
    std::size_t m_NumberThreads = 0;
    std::string m_TemporaryDirectory;
    std::string m_ResultsField;
    std::string m_JobId;
    std::string m_AnalysisName;
    std::string m_MissingFieldValue;
    TStrVec m_CategoricalFieldNames;
    bool m_DiskUsageAllowed;
    // TODO Sparse table support
    // double m_TableLoadFactor = 0.0;
    TRunnerFactoryUPtrVec m_RunnerFactories;
    TRunnerUPtr m_Runner;
    TPersisterSupplier m_PersisterSupplier;
    TRestoreSearcherSupplier m_RestoreSearcherSupplier;
};
}
}

#endif // INCLUDED_ml_api_CDataFrameAnalysisSpecification_h
