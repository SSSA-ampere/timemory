//  MIT License
//
//  Copyright (c) 2020, The Regents of the University of California,
//  through Lawrence Berkeley National Laboratory (subject to receipt of any
//  required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.

/**
 * \file timemory/components/user_bundle/components.hpp
 * \brief Implementation of the user_bundle component(s)
 */

#pragma once

#include "timemory/components/base.hpp"
#include "timemory/mpl/apply.hpp"
#include "timemory/mpl/types.hpp"
#include "timemory/units.hpp"

#include "timemory/components/user_bundle/backends.hpp"
#include "timemory/components/user_bundle/types.hpp"

//======================================================================================//
//
namespace tim
{
namespace component
{
//
//--------------------------------------------------------------------------------------//
//
//                                  USER BUNDLE
//
//--------------------------------------------------------------------------------------//
//
template <size_t Idx, typename Tag>
struct user_bundle : public base<user_bundle<Idx, Tag>, void>
{
public:
    using mutex_t  = std::mutex;
    using lock_t   = std::unique_lock<mutex_t>;
    using string_t = std::string;

    using value_type   = void;
    using this_type    = user_bundle<Idx, Tag>;
    using base_type    = base<this_type, value_type>;
    using storage_type = typename base_type::storage_type;

    using start_func_t  = std::function<void*(const string_t&, bool)>;
    using stop_func_t   = std::function<void(void*)>;
    using get_func_t    = std::function<void(void*, void*&, size_t)>;
    using delete_func_t = std::function<void(void*)>;

    static string_t   label() { return "user_bundle"; }
    static string_t   description() { return "user-defined bundle of tools"; }
    static value_type record() {}

    static void global_init(storage_type*);

    using opaque_array_t = std::vector<opaque>;
    using typeid_set_t   = std::set<size_t>;

    static size_t bundle_size() { return get_data().size(); }

public:
    //----------------------------------------------------------------------------------//
    //  Captures the statically-defined data so these can be changed without
    //  affecting this instance
    //
    user_bundle() = default;

    explicit user_bundle(const string_t& _prefix, bool _flat = false)
    : m_flat(_flat)
    , m_prefix(_prefix)
    {}

    user_bundle(const user_bundle& rhs)
    : base_type(rhs)
    , m_flat(rhs.m_flat)
    , m_prefix(rhs.m_prefix)
    , m_typeids(rhs.m_typeids)
    , m_bundle(rhs.m_bundle)
    {
        for(auto& itr : m_bundle)
            itr.set_copy(true);
    }

    user_bundle(const string_t& _prefix, const opaque_array_t& _bundle_vec,
                bool _flat = false)
    : m_flat(_flat)
    , m_prefix(_prefix)
    , m_bundle(_bundle_vec)
    {}

    ~user_bundle()
    {
        for(auto& itr : m_bundle)
            itr.cleanup();
    }

    user_bundle& operator=(const user_bundle& rhs)
    {
        if(this == &rhs)
            return *this;

        base_type::operator=(rhs);
        m_flat             = rhs.m_flat;
        m_prefix           = rhs.m_prefix;
        m_typeids          = rhs.m_typeids;
        m_bundle           = rhs.m_bundle;
        for(auto& itr : m_bundle)
            itr.set_copy(true);

        return *this;
    }

    user_bundle(user_bundle&&) = default;
    user_bundle& operator=(user_bundle&&) = default;

public:
    //  Configure the tool for a specific component
    static void configure(opaque&& obj, typeid_set_t&& _typeids)
    {
        if(obj)
        {
            lock_t lk(get_lock());
            size_t sum = 0;
            for(auto&& itr : _typeids)
            {
                if(itr > 0 && get_typeids().count(itr) > 0)
                    return;
                sum += itr;
                if(itr > 0)
                    get_typeids().insert(std::move(itr));
            }
            if(sum == 0)
                return;

            internal_init();
            obj.init();
            get_data().emplace_back(std::forward<opaque>(obj));
        }
    }

    template <typename Type, typename... Types, typename... Args>
    static void configure(Args&&... args)
    {
        this_type::configure(factory::get_opaque<Type>(std::forward<Args>(args)...),
                             factory::get_typeids<Type>());

        TIMEMORY_FOLD_EXPRESSION(
            this_type::configure(factory::get_opaque<Types>(std::forward<Args>(args)...),
                                 factory::get_typeids<Types>()));
    }

    //----------------------------------------------------------------------------------//
    //  Explicitly clear the previous configurations
    //
    static void reset()
    {
        lock_t lk(get_lock());
        get_data().clear();
        get_typeids().clear();
    }

public:
    //----------------------------------------------------------------------------------//
    //  Member functions
    //
    void start()
    {
        base_type::set_started();
        for(auto& itr : m_bundle)
            itr.start(m_prefix, m_flat);
    }

    void stop()
    {
        for(auto& itr : m_bundle)
            itr.stop();
        base_type::set_stopped();
    }

    void clear()
    {
        if(base_type::is_running)
            stop();
        m_typeids.clear();
        m_bundle.clear();
    }

    template <typename T>
    T* get()
    {
        auto  _typeid_hash = get_hash(demangle<T>());
        void* void_ptr     = nullptr;
        for(auto& itr : m_bundle)
        {
            itr.get(void_ptr, _typeid_hash);
            if(void_ptr)
                return void_ptr;
        }
        return static_cast<T*>(void_ptr);
    }

    void get(void*& ptr, size_t _hash) const
    {
        for(const auto& itr : m_bundle)
        {
            itr.get(ptr, _hash);
            if(ptr)
                break;
        }
    }

    void get() {}

    void set_prefix(const string_t& _prefix) { m_prefix = _prefix; }

    size_t size() const { return m_bundle.size(); }

public:
    //  Configure the tool for a specific component
    void insert(opaque&& obj, typeid_set_t&& _typeids)
    {
        if(obj)
        {
            size_t sum = 0;
            for(auto&& itr : _typeids)
            {
                if(itr > 0 && m_typeids.count(itr) > 0)
                    return;
                sum += itr;
                m_typeids.insert(std::move(itr));
            }
            if(sum == 0)
                return;

            internal_init();
            obj.init();
            m_bundle.emplace_back(std::forward<opaque>(obj));
        }
    }

    template <typename Type, typename... Types, typename... Args>
    void insert(Args&&... args)
    {
        this->insert(factory::get_opaque<Type>(std::forward<Args>(args)...),
                     factory::get_typeids<Type>());

        TIMEMORY_FOLD_EXPRESSION(
            this->insert(factory::get_opaque<Types>(std::forward<Args>(args)...),
                         factory::get_typeids<Types>()));
    }

    void set_flat_profile(bool val) { m_flat = val; }

protected:
    bool           m_flat    = false;
    string_t       m_prefix  = "";
    typeid_set_t   m_typeids = get_typeids();
    opaque_array_t m_bundle  = get_data();

private:
    struct persistent_data
    {
        mutex_t        lock;
        opaque_array_t data    = {};
        typeid_set_t   typeids = {};
    };

    //----------------------------------------------------------------------------------//
    //  Persistent data
    //
    static persistent_data& get_persistent_data()
    {
        static persistent_data _instance{};
        return _instance;
    }

    //----------------------------------------------------------------------------------//
    //  Bundle data
    //
    static opaque_array_t& get_data() { return get_persistent_data().data; }

    //----------------------------------------------------------------------------------//
    //  The configuration strings
    //
    static typeid_set_t& get_typeids() { return get_persistent_data().typeids; }

    //----------------------------------------------------------------------------------//
    //  Get lock
    //
    static mutex_t& get_lock() { return get_persistent_data().lock; }

    //----------------------------------------------------------------------------------//
    //  Initialize the storage
    //
    static void internal_init()
    {
        static bool _inited = []() {
            auto ret = storage_type::instance();
            ret->initialize();
            return true;
        }();
        consume_parameters(_inited);
    }
};
//
}  // namespace component
}  // namespace tim
//
//======================================================================================//
