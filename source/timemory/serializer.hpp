//  MIT License
//
//  Copyright (c) 2018, The Regents of the University of California,
//  through Lawrence Berkeley National Laboratory (subject to receipt of any
//  required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//  IN THE SOFTWARE.

/** \file serializer.hpp
 * \headerfile serializer.hpp "timemory/serializer.hpp"
 * Headers for serialization
 */

#pragma once

#include "timemory/macros.hpp"

#include <fstream>
#include <initializer_list>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <type_traits>

#include <deque>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <cereal/access.hpp>
#include <cereal/cereal.hpp>
#include <cereal/macros.hpp>

#include <cereal/types/chrono.hpp>
#include <cereal/types/deque.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>

#include <cereal/archives/adapters.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>

//======================================================================================//

namespace serializer
{
//--------------------------------------------------------------------------------------//

using cereal::make_nvp;

//--------------------------------------------------------------------------------------//

}  // namespace serializer

namespace cereal
{
//--------------------------------------------------------------------------------------//
//  save specialization for pair of string and type
//
template <typename Archive, typename Type,
          traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void
save(Archive& ar, const std::pair<std::string, Type>& p)
{
    ar(cereal::make_nvp(p.first, p.second));
}

//--------------------------------------------------------------------------------------//
//  load specialization for pair of string and type
//
template <class Archive, typename Type,
          traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void
load(Archive& ar, std::pair<std::string, Type>& p)
{
    const auto key = ar.getNodeName();
    p.first        = key;
    ar(p.second);
}

//--------------------------------------------------------------------------------------//
//  save specialization for pair
//
template <typename Archive, typename TypeF, typename TypeS,
          traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void
save(Archive& ar, const std::pair<TypeF, TypeS>& p)
{
    ar(cereal::make_nvp("first", p.first), cereal::make_nvp("second", p.second));
}

//--------------------------------------------------------------------------------------//
//  load specialization for pair
//
template <class Archive, typename TypeF, typename TypeS,
          traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void
load(Archive& ar, std::pair<TypeF, TypeS>& p)
{
    ar(p.first, p.second);
}

//--------------------------------------------------------------------------------------//
//  save specialization for pair
//
template <typename Archive, typename Type, std::size_t Size,
          traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void
save(Archive& ar, const std::array<Type, Size>& arr)
{
    std::vector<Type> vec(Size, Type());
    for(std::size_t i = 0; i < Size; ++i)
        vec[i] = arr[i];
    ar(vec);
}

//--------------------------------------------------------------------------------------//
//  load specialization for pair
//
template <typename Archive, typename Type, std::size_t Size,
          traits::EnableIf<traits::is_text_archive<Archive>::value> = traits::sfinae>
inline void
load(Archive& ar, std::array<Type, Size>& arr)
{
    std::vector<Type> vec(Size, Type());
    ar(vec);
    for(std::size_t i = 0; i < Size; ++i)
        arr[i] = vec[i];
}

//--------------------------------------------------------------------------------------//

}  // namespace cereal

//======================================================================================//
