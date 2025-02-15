// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcistrmb.h>
#include <dcmtk/dcmdata/dcostrma.h>
#include <dcmtk/dcmdata/dcostrmb.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/ofstd/ofvector.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <absl/strings/string_view.h>

#include <memory>
#include <utility>
#include <vector>
#include <string>

#include "src/dcmFileDraft.h"
#include "src/frame.h"
#include "tests/test_frame.h"
#include "tests/testUtils.h"


namespace wsiToDicomConverter {

static int bufferSize = 10000;

TEST(fileGeneration, withoutConcatenation) {
  std::vector<std::unique_ptr<AbstractDcmFile>> empty_dicom_file_vec;
  // emptyPixelData
  std::vector<std::unique_ptr<Frame>> framesData;
  for (int idx = 0; idx < 100; ++idx) {
      framesData.push_back(std::make_unique<TestFrame>(500, 500));
  }
  DcmFileDraft draft(std::move(framesData), "./", 50000, 50000, 0, 0, 0,
                "study", "series", "image", JPEG, false, nullptr, 0.0, 0.0, 1,
                 &empty_dicom_file_vec, "FileGeneration without concatenation",
                 true);

  OFVector<Uint8> writeBuffer(bufferSize);
  std::unique_ptr<DcmOutputBufferStream> output =
      std::make_unique<DcmOutputBufferStream>(&writeBuffer[0],
                                              writeBuffer.size());
  draft.write(output.get());
  DcmFileFormat dcmFileFormat;
  DcmInputBufferStream input;
  input.setBuffer(&writeBuffer[0], writeBuffer.size());
  dcmFileFormat.read(input);
  char* stringValue;
  findElement(dcmFileFormat.getDataset(), DCM_LossyImageCompression)
      ->getString(stringValue);

  EXPECT_EQ("01", absl::string_view(stringValue));
  findElement(dcmFileFormat.getDataset(), DCM_SeriesDescription)
      ->getString(stringValue);
  EXPECT_EQ("image", absl::string_view(stringValue));

  DcmSequenceOfItems* element =
      reinterpret_cast<DcmSequenceOfItems*>(findElement(
          dcmFileFormat.getDataset(), DCM_PerFrameFunctionalGroupsSequence));

  EXPECT_NE(nullptr, element);
}

TEST(fileGeneration, withConcatenation) {
  std::vector<std::unique_ptr<AbstractDcmFile>> dicom_file_vec;
  std::vector<std::unique_ptr<Frame>> framesData;
  for (int idx = 0; idx < 50; ++idx) {
      framesData.push_back(std::make_unique<TestFrame>(500, 500));
  }
  std::unique_ptr<DcmFileDraft> batch_0_dicom = std::make_unique<DcmFileDraft>(
      std::move(framesData), "./", 50000, 50000, 0, 0, 0,
      "study", "series", "image", RAW, true, nullptr, 0.0, 0.0, 1,
      &dicom_file_vec, "FileGeneration with concatenation 1", true);
  dicom_file_vec.push_back(std::move(batch_0_dicom));

  // emptyPixelData
  for (int idx = 0; idx < 50; ++idx) {
      framesData.push_back(std::make_unique<TestFrame>(500, 500));
  }
  DcmFileDraft draft(std::move(framesData), "./", 50000, 50000, 0, 0, 0,
      "study", "series", "image", RAW, true, nullptr, 0.0, 0.0, 1,
      &dicom_file_vec, "FileGeneration with concatenation 2", true);

  OFVector<Uint8> writeBuffer(bufferSize);
  std::unique_ptr<DcmOutputBufferStream> output =
      std::make_unique<DcmOutputBufferStream>(&writeBuffer[0],
                                              writeBuffer.size());
  draft.write(output.get());
  DcmFileFormat dcmFileFormat;
  DcmInputBufferStream input;
  input.setBuffer(&writeBuffer[0], writeBuffer.size());
  dcmFileFormat.read(input);
  char* stringValue;
  Uint16 batchNumber;
  findElement(dcmFileFormat.getDataset(), DCM_InConcatenationNumber)
      ->getUint16(batchNumber);
  EXPECT_EQ(2, batchNumber);
  findElement(dcmFileFormat.getDataset(), DCM_LossyImageCompression)
      ->getString(stringValue);
  EXPECT_EQ("00", absl::string_view(stringValue));
}

TEST(fileGeneration, fileSave) {
  // emptyPixelData
  std::vector<std::unique_ptr<Frame>> framesData;
  for (int idx = 0; idx < 100; ++idx) {
      framesData.push_back(std::make_unique<TestFrame>(500, 500));
  }
  DcmFileDraft draft(std::move(framesData), "./", 50000, 50000, 0, 0, 0,
      "study", "series", "image", JPEG2000, true, nullptr, 0.0, 0.0, 1, NULL,
      "FileGeneration fileSave", true);

  draft.saveFile();
  ASSERT_TRUE(boost::filesystem::exists("./downsample-1-frames-0-100.dcm"));
}

TEST(fileGeneration, fileSaveBatch) {
  // emptyPixelData
  std::vector<std::unique_ptr<AbstractDcmFile>> dicom_file_vec;
  std::vector<std::unique_ptr<Frame>> framesData;
  for (int count = 0; count < 9; ++count) {
    for (int idx = 0; idx < 100; ++idx) {
      framesData.push_back(std::make_unique<TestFrame>(50, 50, 1));
    }
    std::unique_ptr<DcmFileDraft> draft = std::make_unique<DcmFileDraft>(
       std::move(framesData), "./", 50000, 50000, 2, 0, 0, "study", "series",
       "image", JPEG2000, true, nullptr, 0.0, 0.0, 1, &dicom_file_vec,
       "FileGeneration fileSaveBatch 1", true);
    dicom_file_vec.push_back(std::move(draft));
  }

  for (int idx = 0; idx < 100; ++idx) {
      framesData.push_back(std::make_unique<TestFrame>(50, 50, 1));
  }
  DcmFileDraft draft(std::move(framesData), "./", 50000, 50000, 2, 0,
     0, "study", "series", "image", JPEG2000, true, nullptr, 0.0, 0.0, 1,
     &dicom_file_vec, "FileGeneration fileSaveBatch 2", true);

  draft.saveFile();
  ASSERT_TRUE(boost::filesystem::exists("./downsample-1-frames-900-1000.dcm"));
}

}  // namespace wsiToDicomConverter
