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

#include "CDataFrameMockAnalysisRunner.h"

#include <core/CDataFrame.h>
#include <core/CLoopProgress.h>
#include <core/CPackedBitVector.h>

CDataFrameMockAnalysisRunner::CDataFrameMockAnalysisRunner(const ml::api::CDataFrameAnalysisSpecification& spec)
    : ml::api::CDataFrameAnalysisRunner{spec}, m_Instrumentation{spec.jobId()} {
    this->computeAndSaveExecutionStrategy();
}

std::size_t CDataFrameMockAnalysisRunner::numberExtraColumns() const {
    return 2;
}

std::size_t CDataFrameMockAnalysisRunner::dataFrameSliceCapacity() const {
    return 10000;
}

ml::core::CPackedBitVector
CDataFrameMockAnalysisRunner::rowsToWriteMask(const ml::core::CDataFrame& frame) const {
    return {frame.numberRows(), true};
}

void CDataFrameMockAnalysisRunner::writeOneRow(const ml::core::CDataFrame&,
                                               const TRowRef&,
                                               ml::core::CRapidJsonConcurrentLineWriter&) const {
}

bool CDataFrameMockAnalysisRunner::validate(const ml::core::CDataFrame&) const {
    return true;
}

void CDataFrameMockAnalysisRunner::runImpl(ml::core::CDataFrame&) {
    ml::core::CLoopProgress progress{31, this->instrumentation().progressCallback()};
    for (std::size_t i = 0; i < 31; ++i, progress.increment()) {
        std::vector<std::size_t> wait;
        ms_Rng.generateUniformSamples(1, 20, 1, wait);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait[0]));
    }
}

std::size_t CDataFrameMockAnalysisRunner::estimateBookkeepingMemoryUsage(std::size_t,
                                                                         std::size_t,
                                                                         std::size_t,
                                                                         std::size_t) const {
    return 0;
}

const ml::api::CDataFrameAnalysisInstrumentation&
CDataFrameMockAnalysisRunner::instrumentation() const {
    return m_Instrumentation;
}

ml::api::CDataFrameAnalysisInstrumentation& CDataFrameMockAnalysisRunner::instrumentation() {
    return m_Instrumentation;
}

ml::test::CRandomNumbers CDataFrameMockAnalysisRunner::ms_Rng;

const std::string& CDataFrameMockAnalysisRunnerFactory::name() const {
    return NAME;
}

CDataFrameMockAnalysisRunnerFactory::TRunnerUPtr
CDataFrameMockAnalysisRunnerFactory::makeImpl(const ml::api::CDataFrameAnalysisSpecification& spec,
                                              TDataFrameUPtrTemporaryDirectoryPtrPr*) const {
    return std::make_unique<CDataFrameMockAnalysisRunner>(spec);
}

CDataFrameMockAnalysisRunnerFactory::TRunnerUPtr
CDataFrameMockAnalysisRunnerFactory::makeImpl(const ml::api::CDataFrameAnalysisSpecification& spec,
                                              const rapidjson::Value&,
                                              TDataFrameUPtrTemporaryDirectoryPtrPr*) const {
    return std::make_unique<CDataFrameMockAnalysisRunner>(spec);
}

const std::string CDataFrameMockAnalysisRunnerFactory::NAME{"test"};

ml::counter_t::ECounterTypes CDataFrameMockAnalysisState::memoryCounterType() {
    return ml::counter_t::E_DFOPeakMemoryUsage;
}
