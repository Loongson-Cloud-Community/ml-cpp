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

#ifndef INCLUDED_CDataFrameMockAnalysisRunner_h
#define INCLUDED_CDataFrameMockAnalysisRunner_h

#include <core/CDataFrame.h>

#include <api/CDataFrameAnalysisInstrumentation.h>
#include <api/CDataFrameAnalysisRunner.h>
#include <api/CDataFrameAnalysisSpecification.h>

#include <test/CRandomNumbers.h>

#include <cinttypes>
#include <string>

class CDataFrameMockAnalysisState final : public ml::api::CDataFrameAnalysisInstrumentation {
public:
    CDataFrameMockAnalysisState(const std::string& jobId)
        : ml::api::CDataFrameAnalysisInstrumentation(jobId, 0ul) {}
    void writeAnalysisStats(std::int64_t /* timestamp */) override {}

protected:
    ml::counter_t::ECounterTypes memoryCounterType() override;
};

class CDataFrameMockAnalysisRunner final : public ml::api::CDataFrameAnalysisRunner {
public:
    explicit CDataFrameMockAnalysisRunner(const ml::api::CDataFrameAnalysisSpecification& spec);

    std::size_t numberExtraColumns() const override;
    std::size_t dataFrameSliceCapacity() const override;
    ml::core::CPackedBitVector rowsToWriteMask(const ml::core::CDataFrame& frame) const override;
    void writeOneRow(const ml::core::CDataFrame&,
                     const TRowRef&,
                     ml::core::CRapidJsonConcurrentLineWriter&) const override;
    bool validate(const ml::core::CDataFrame&) const override;
    const ml::api::CDataFrameAnalysisInstrumentation& instrumentation() const override;
    ml::api::CDataFrameAnalysisInstrumentation& instrumentation() override;

private:
    void runImpl(ml::core::CDataFrame&) override;
    std::size_t estimateBookkeepingMemoryUsage(std::size_t,
                                               std::size_t,
                                               std::size_t,
                                               std::size_t) const override;

private:
    static ml::test::CRandomNumbers ms_Rng;
    CDataFrameMockAnalysisState m_Instrumentation;
};

class CDataFrameMockAnalysisRunnerFactory final : public ml::api::CDataFrameAnalysisRunnerFactory {
public:
    const std::string& name() const override;

private:
    static const std::string NAME;

private:
    TRunnerUPtr makeImpl(const ml::api::CDataFrameAnalysisSpecification& spec,
                         TDataFrameUPtrTemporaryDirectoryPtrPr*) const override;
    TRunnerUPtr makeImpl(const ml::api::CDataFrameAnalysisSpecification& spec,
                         const rapidjson::Value&,
                         TDataFrameUPtrTemporaryDirectoryPtrPr*) const override;
};

#endif // INCLUDED_CDataFrameMockAnalysisRunner_h
