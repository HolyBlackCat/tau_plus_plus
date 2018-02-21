#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <any>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <SDL2/SDL_endian.h>
#include <zlib.h>

#include "exceptions.h"
#include "reflection.h"
#include "template_utils.h"

namespace Utils
{
    namespace impl
    {
        template <typename T, typename = void> struct has_custom_null : std::false_type {};
        template <typename T> struct has_custom_null<T, std::void_t<decltype(T::Null())>> : std::true_type {};
    }

    /* T should contain:
     * static Handle Create(Params...)
     * static void Destroy(Handle) // The parameter should be compatible with return type of `Create()`.
     * static void Error(Params...) // Parameters should be compatible with those of `Create()`.
     * Optional:
     * static Handle Null()
     * static void Move(const Handle *old, const Handle *new) // This will be executed after a normal move.
     */
    template <typename T> class Handle
    {
      public:
        using handle_t = TemplateUtils::return_type<decltype(T::Create)>;
        using params_t = TemplateUtils::param_types<decltype(T::Create)>;
      private:
        handle_t handle;
      public:
        [[nodiscard]] bool is_null() const noexcept
        {
            return handle == null();
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return !is_null();
        }

        [[nodiscard]] const handle_t &value() const noexcept
        {
            return handle;
        }

        [[nodiscard]] const handle_t &operator *() const noexcept
        {
            return handle;
        }

        [[nodiscard]] Handle &&move() noexcept
        {
            return (Handle &&)*this;
        }

        void create(const params_t &params)
        {
            destroy();
            handle = std::apply(T::Create, params);
            if (is_null())
                std::apply(T::Error, params);
        }

        void destroy() noexcept
        {
            if (!is_null())
            {
                T::Destroy(handle);
                handle = null();
            }
        }

        [[nodiscard]] static handle_t null() noexcept
        {
            // This `if` is contrived way to say `if constexpr (CustomNull)` that dodges `CustomNull can't be null` warning.
            if constexpr (impl::has_custom_null<T>::value)
                return T::Null();
            else
                return handle_t(0);
        }

        Handle() noexcept : handle(null()) {}

        Handle(const params_t &params)
        {
            handle = std::apply(T::Create, params);
            if (is_null())
                std::apply(T::Error, params);
        }

        Handle(const Handle &) = delete;

        Handle(Handle &&o) noexcept(noexcept(handle_t(o.handle)) && noexcept(o.handle = null())) : handle(o.handle)
        {
            o.handle = null();
        }

        Handle &operator=(const Handle &) = delete;

        Handle &operator=(Handle &&o) noexcept(noexcept(handle = o.handle) && noexcept(o.handle = null()))
        {
            if (&o == this)
                return *this;
            destroy();
            handle = o.handle;
            o.handle = null();
            return *this;
        }

        Handle &operator=(const params_t &params)
        {
            create(params);
            return *this;
        }

        ~Handle() noexcept
        {
            destroy();
        }
    };

    template <typename T> class AutoPtr
    {
        T *ptr;
        template <typename TT, typename ...P> void alloc_without_free(P &&... p)
        {
            ptr = new TT((P &&)p...);
            if (!ptr) // Just to be sure.
                throw std::bad_alloc{};
        }
      public:
        template <typename TT, typename ...P> void alloc_t(P &&... p)
        {
            free();
            alloc_without_free<TT>((P &&)p...);
        }
        template <typename ...P> void alloc(P &&... p)
        {
            alloc_t<T>((P &&)p...);
        }
        void free()
        {
            if (ptr)
            {
                std::default_delete<T>{}(ptr);
                ptr = 0;
            }
        }

        [[nodiscard]] bool is_null() const noexcept
        {
            return !ptr;
        }
        [[nodiscard]] operator T *() const noexcept
        {
            return ptr;
        }
        [[nodiscard]] T &operator*() const noexcept
        {
            return *ptr;
        }
        [[nodiscard]] T *operator->() const noexcept
        {
            return ptr;
        }

        AutoPtr() noexcept : ptr(0) {}

        template <typename ...P> AutoPtr(P &&... p) : ptr(0)
        {
            alloc_without_free<T>((P &&)p...);
        }

        AutoPtr(const AutoPtr &) = delete;

        AutoPtr(AutoPtr &&o) noexcept : ptr(o.ptr)
        {
            o.ptr = 0;
        }

        AutoPtr &operator=(const AutoPtr &) = delete;

        AutoPtr &operator=(AutoPtr &&o) noexcept
        {
            if (&o == this)
                return *this;

            std::default_delete<T>{}(ptr);
            ptr = o.ptr;
            o.ptr = 0;
            return *this;
        }

        ~AutoPtr()
        {
            free();
        }
    };


    template <typename Res = int, typename Index = int> class ResourceAllocator
    {
        static_assert(std::is_integral<Res>::value && std::is_integral<Index>::value, "Integral types must be used.");

        Index pos;
        std::vector<Res> pool;
        std::vector<Index> locations;

        using ResIterator = typename std::vector<Res>::const_iterator;
      public:
        inline static const Res not_allocated = -1;

        ResourceAllocator(Index pool_size = 0)
        {
            resize(pool_size);
        }

        void resize(Index new_size) // Frees all resources.
        {
            // Extra <s>useless</s> protection agains exceptions.
            std::vector<Res> new_pool(new_size);
            std::vector<Index> new_locations(new_size);
            pool = std::move(new_pool);
            locations = std::move(new_locations);

            pos = 0;
            for (Index i = 0; i < new_size; i++)
            {
                pool[i] = Index(i);
                locations[i] = Index(i);
            }
        }

        Res alloc() // Returns `not_allocated` (aka -1) on failure.
        {
            if (pos >= Index(pool.size()))
                return not_allocated;
            return pool[pos++];
        }
        bool free(Res id) // Returns 0 if such id was not allocated before.
        {
            if (id < 0 || id >= Res(pool.size()) || locations[id] >= pos)
                return 0;
            pos--;
            Res last_id = pool[pos];
            std::swap(pool[locations[id]], pool[pos]);
            std::swap(locations[id], locations[last_id]);
            return 1;
        }
        void free_everything()
        {
            pos = 0;
        }
        Index max_size() const
        {
            return Index(pool.size());
        }
        Index current_size() const
        {
            return pos;
        }

        ResIterator begin_all() const
        {
            return pool.begin();
        }
        ResIterator end_all() const
        {
            return pool.end();
        }
        ResIterator begin_allocated() const
        {
            return begin_all();
        }
        ResIterator end_allocated() const
        {
            return pool.begin() + pos;
        }
        ResIterator begin_free() const
        {
            return end_allocated();
        }
        ResIterator end_free() const
        {
            return end_all();
        }
    };


    inline namespace Ranges
    {
        namespace impl
        {
            template <typename T, typename = void> struct has_contiguous_storage : std::false_type {};
            template <typename T> struct has_contiguous_storage<T, std::void_t<decltype(std::data(std::declval<const T &>()))>> : std::true_type {};

            template <typename T, typename = void> struct has_std_size : std::false_type {};
            template <typename T> struct has_std_size<T, std::void_t<decltype(std::size(std::declval<const T &>()))>> : std::true_type {};
        }

        enum Storage {any, contiguous};

        template <typename T, Storage S = any> class Range
        {
            struct FuncTable
            {
                void (*copy)(const std::any &from, std::any &to);
                bool (*equal)(const std::any &a, const std::any &b);
                void (*increment)(std::any &iter);
                std::ptrdiff_t (*distance)(const std::any &a, const std::any &b);
                T &(*dereference)(const std::any &iter);
            };

            class Iterator
            {
                std::any data;
                const FuncTable *table;
              public:
                template <typename Iter> Iterator(const Iter &source)
                {
                    static FuncTable table_storage
                    {
                        [](const std::any &from, std::any &to) // copy
                        {
                            to.emplace<Iter>(std::any_cast<const Iter &>(from));
                        },
                        [](const std::any &a, const std::any &b) -> bool // equal
                        {
                            return std::any_cast<const Iter &>(a) == std::any_cast<const Iter &>(b);
                        },
                        [](std::any &iter) // increment
                        {
                            ++std::any_cast<Iter &>(iter);
                        },
                        [](const std::any &a, const std::any &b) -> std::ptrdiff_t // distance
                        {
                            return std::distance(std::any_cast<const Iter &>(a), std::any_cast<const Iter &>(b));
                        },
                        [](const std::any &iter) -> T & // dereference
                        {
                            return *std::any_cast<const Iter &>(iter);
                        },
                    };

                    data.emplace<Iter>(source);
                    table = &table_storage;
                }

                Iterator() : table(0) {}
                Iterator(const Iterator &other)
                {
                    table = other.table;
                    table->copy(other.data, data);
                }
                Iterator &operator=(const Iterator &other)
                {
                    if (&other == this)
                        return *this;
                    table = other.table;
                    table->copy(other.data, data);
                    return *this;
                }

                using value_type = T;
                using reference  = T &;
                using pointer    = T *;
                using difference_type   = std::ptrdiff_t;
                using iterator_category = std::forward_iterator_tag;

                reference operator*() const
                {
                    return table->dereference(data);
                }
                pointer operator->() const
                {
                    return &table->dereference(data);
                }
                bool operator==(const Iterator &other) const
                {
                    if (table != other.table) return 0;
                    if (table == 0) return 1;
                    return table->equal(data, other.data);
                }
                bool operator!=(const Iterator &other) const
                {
                    return !(*this == other);
                }
                Iterator &operator++()
                {
                    table->increment(data);
                    return *this;
                }
                Iterator operator++(int)
                {
                    Iterator ret = *this;
                    ++*this;
                    return ret;
                }
                difference_type operator-(const Iterator &other) const
                {
                    return table->distance(other.data, data);
                }
            };

            Iterator begin_iter, end_iter;
            mutable std::size_t len = -1;
          public:
            inline static constexpr bool is_const = std::is_const_v<T>;

            Range() : Range((T *)0, (T *)0) {}
            template <typename Object, typename = std::void_t<std::enable_if_t<!std::is_same_v<Object, Range>>,
                                                              decltype(std::begin(std::declval<Object &>())),
                                                              decltype(std::end  (std::declval<Object &>()))>>
            Range(const Object &obj) : Range(std::begin(obj), std::end(obj))
            {
                static_assert(S != contiguous || impl::has_contiguous_storage<Object>::value, "The object must have a contiguous storage.");
                if constexpr (impl::has_std_size<Object>::value)
                    len = std::size(obj);
            }
            Range(std::initializer_list<T> list) : Range(&*list.begin(), &*list.end()) {}
            template <typename Iter> Range(const Iter &begin, const Iter &end) : begin_iter(begin), end_iter(end)
            {
                static_assert(S != contiguous || std::is_pointer_v<Iter>, "For contiguous storage iterators have to be pointers.");
            }
            Iterator begin() const
            {
                return begin_iter;
            }
            Iterator end() const
            {
                return end_iter;
            }
            std::size_t size() const
            {
                if (len == std::size_t(-1))
                    len = end_iter - begin_iter;
                return len;
            }
        };
        template <typename T> using ViewRange = Range<const T>;
        template <typename T> using ContiguousRange = Range<T, contiguous>;
        template <typename T> using ViewContiguousRange = Range<const T, contiguous>;
    }


    DefineExceptionInline(file_input_error, "File loading error.",
        (std::string,name,"File name")
        (std::string,message,"Message")
    )
    inline namespace Files
    {
        namespace impl
        {
            class FileHandleFuncs
            {
                template <typename> friend class ::Utils::Handle;
                static FILE *Create(const char *fname, const char *mode) {return fopen(fname, mode);}
                static void Destroy(FILE *value) {fclose(value);}
                static void Error(const char *fname, const char *) {throw file_input_error(fname, "Unable to open.");}
            };
            using FileHandle = Handle<FileHandleFuncs>;
        }

        enum Compression {not_compressed, compressed};

        class MemoryFile // Manages a ref-counted memory copy of a file. The data is always null-terminated, '\0' doesn't count against size.
        {
            struct Object
            {
                std::string name;
                std::size_t size;
                std::unique_ptr<uint8_t[]> bytes;
            };
            std::shared_ptr<Object> data;

          public:
            MemoryFile() {}

            MemoryFile(std::string fname, Compression mode = not_compressed)
            {
                Create(fname, mode);
            }
            MemoryFile(const char *fname, Compression mode = not_compressed)
            {
                Create(fname, mode);
            }
            void Create(std::string fname, Compression mode = not_compressed)
            {
                impl::FileHandle input({fname.c_str(), "rb"});

                std::fseek(*input, 0, SEEK_END);
                auto size = std::ftell(*input);
                std::fseek(*input, 0, SEEK_SET);
                if (std::ferror(*input) || size == EOF)
                    throw file_input_error(fname, "Unable to get file size.");

                auto buf = std::make_unique<uint8_t[]>(mode == not_compressed ? size+1 : size); // +1 to make space for '\0' if it's not compressed.
                if (!std::fread(buf.get(), size, 1, *input))
                    throw file_input_error(fname, "Unable to read.");

                switch (mode)
                {
                  case not_compressed:
                    buf[size] = '\0';
                    data = std::make_shared<Object>(Object{fname, (std::size_t)size, std::move(buf)});
                    break;
                  case compressed:
                    {
                        uint32_t uncompr_size;
                        if (!Reflection::from_bytes(uncompr_size, buf.get(), buf.get() + size))
                            throw file_input_error(fname, "Unable to decompress (too small).");

                        auto uncompr_buf = std::make_unique<uint8_t[]>(uncompr_size+1); // +1 to make space for '\0'.
                        uncompr_buf[uncompr_size] = '\0';
                        uLongf uncompr_size_real = uncompr_size;
                        if (uncompress(uncompr_buf.get(), &uncompr_size_real, buf.get()+4, size-4) != Z_OK)
                            throw file_input_error(fname, "Unable to decompress.");
                        if (uncompr_size != uncompr_size_real)
                            throw file_input_error(fname, "Compressed data size mismatch.");

                        data = std::make_shared<Object>(Object{fname, (std::size_t)uncompr_size, std::move(uncompr_buf)});
                    }
                    break;
                }
            }
            void Destroy()
            {
                data.reset();
            }
            bool Exists() const
            {
                return bool(data);
            }
            const uint8_t *Data() const
            {
                return data->bytes.get();
            }
            std::size_t Size() const
            {
                return data->size;
            }
            const std::string &Name() const
            {
                return data->name;
            }
        };

        inline bool WriteToFile(std::string fname, const uint8_t *buf, std::size_t len, Compression mode = not_compressed)
        {
            try
            {
                impl::FileHandle output({fname.c_str(), "wb"});

                switch (mode)
                {
                  case not_compressed:
                    if (!std::fwrite(buf, len, 1, *output))
                        return 0;
                    break;
                  case compressed:
                    {
                        auto compr_len = compressBound(len);
                        auto compr_buf = std::make_unique<uint8_t[]>(compr_len);
                        if (compress(compr_buf.get(), &compr_len, buf, len) != Z_OK)
                            return 0;

                        uint32_t out_len = len;
                        Reflection::Bytes::fix_order(out_len);
                        if (!std::fwrite(&out_len, sizeof out_len, 1, *output))
                            return 0;

                        if (!std::fwrite(compr_buf.get(), compr_len, 1, *output))
                            return 0;
                    }
                    break;
                }
            }
            catch (decltype(file_input_error("","")) &e)
            {
                return 0;
            }
            return 1;
        }
    }


    inline namespace ByteOrder
    {
        inline constexpr bool big_endian    = SDL_BYTEORDER == SDL_BIG_ENDIAN,
                              little_endian = SDL_BYTEORDER == SDL_LIL_ENDIAN;

        template <typename T> void SwapBytesInPlace(T &value)
        {
            static_assert(std::is_arithmetic_v<T>, "You probably don't want to use this for non-arithmetic types.");
            if constexpr (sizeof(T) == 1) return;
            else if constexpr (sizeof(T) == 2) {auto tmp = SDL_Swap16((uint16_t &)value); value = (T &)tmp;}
            else if constexpr (sizeof(T) == 4) {auto tmp = SDL_Swap32((uint32_t &)value); value = (T &)tmp;}
            else if constexpr (sizeof(T) == 8) {auto tmp = SDL_Swap64((uint64_t &)value); value = (T &)tmp;}

            char (&ref)[sizeof(T)] = (char (&)[sizeof(T)])value;

            for (std::size_t i = 0; i < sizeof(T)/2; i++)
                ref[i] = ref[sizeof(T) - 1 - i];
        }
        template <typename T> [[nodiscard]] T SwapBytes(T value)
        {
            SwapBytesInPlace(value);
            return value;
        }

        template <typename T> void MakeLittle([[maybe_unused]] T &value)
        {
            static_assert(std::is_arithmetic_v<T>, "You probably don't want to use this for non-arithmetic types.");
            if constexpr (big_endian)
                SwapBytesInPlace(value);
        }
        template <typename T> void MakeBig([[maybe_unused]] T value)
        {
            static_assert(std::is_arithmetic_v<T>, "You probably don't want to use this for non-arithmetic types.");
            if constexpr (little_endian)
                SwapBytesInPlace(value);
        }

        template <typename T> [[nodiscard]] T Little(T value)
        {
            MakeLittle(value);
            return value;
        }
        template <typename T> [[nodiscard]] T Big(T value)
        {
            MakeBig(value);
            return value;
        }
    }
}

#endif
