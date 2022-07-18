//
// Created by wangrl2016 on 2022/7/18.
//

#ifndef MULTIMEDIA_AUDIO_SAMPLE_TYPES_H
#define MULTIMEDIA_AUDIO_SAMPLE_TYPES_H

// To specify different sample formats, we provide a class for each sample
// format that knows certain things about it, such as the C++ data type used
// to store sample values, mix and max values, as well as how to convert to
// and from floating point formats. Each class must satisfy a concept we call
// "SampleTypeTraits", which requires that the following public functions
// are provided:
//   * A type |ValueType| specifying the C++ type for storing sample values
//   * A static constant kMinValue which specifies the minimum sample value
//   * A static constant kMaxValue which specifies the maximum sample value
//   * A static constant kZeroPointValue which specifies the sample value
//     representing an amplitude of zero
//   * A static method ConvertFromFloat() that takes a float sample value and
//     converts it to the corresponding ValueType
//   * A static method ConvertFromDouble() that takes a double sample value and
//     converts it to the corresponding ValueType
//   * A static method ConvertToFloat() that takes a ValueType sample value and
//     converts it to the corresponding float value
//   * A static method ConvertToDouble() that takes a ValueType sample value and
//     convert it to the corresponding double value

namespace mm {

}

#endif //MULTIMEDIA_AUDIO_SAMPLE_TYPES_H
