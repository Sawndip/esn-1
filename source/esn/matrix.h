#ifndef __ESN_SOURCE_MATRIX_H__
#define __ESN_SOURCE_MATRIX_H__

#include <esn/pointer.h>
#include <vector>

namespace ESN {

    template <class T>
    class matrix
    {
    public:
        matrix(
            std::size_t rows,
            std::size_t cols)
            : m_rows(rows)
            , m_cols(cols)
            , m_ld(rows)
            , m_ptr(make_pointer<T>(rows * cols))
            , m_off(0)
        {}

        matrix(
            const pointer<T> & ptr,
            std::size_t rows,
            std::size_t cols,
            std::size_t ld,
            std::size_t off = 0)
            : m_rows(rows)
            , m_cols(cols)
            , m_ld(ld)
            , m_ptr(ptr)
            , m_off(off)
        {}

        matrix(
            const std::vector<T> & v,
            std::size_t rows,
            std::size_t cols)
            : m_rows(rows)
            , m_cols(cols)
            , m_ld(rows)
            , m_ptr(make_pointer<T>(rows * cols))
            , m_off(0)
        {
            memcpy(m_ptr, v);
        }

        std::size_t rows() const { return m_rows; }
        std::size_t cols() const { return m_cols; }
        std::size_t ld() const { return m_ld; }
        const pointer<T> & ptr() { return m_ptr; }
        const_pointer<T> ptr() const { return m_ptr; }
        T * data() { return m_ptr.get() + m_off; }
        const T * data() const { return m_ptr.get() + m_off; }

    private:
        std::size_t m_rows;
        std::size_t m_cols;
        std::size_t m_ld;
        pointer<T> m_ptr;
        std::size_t m_off;
    };

} // namespace ESN

#endif // __ESN_SOURCE_MATRIX_H__