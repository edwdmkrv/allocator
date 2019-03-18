#ifndef __ALLOCATOR_INCLUDED
#define __ALLOCATOR_INCLUDED

#include <cstdint>
#include <cstdlib>

#include <new>
#include <stdexcept>
#include <memory>
#include <utility>
#include <limits>
#include <type_traits>

namespace usr {

/* Allocator stuff
 */

namespace internal {

template <typename X, std::size_t N>
class SpecialAllocator {
private:
	using idx_t =
		typename std::conditional<N <= std::numeric_limits<uint8_t>::max(),
			uint8_t,
			typename std::conditional<N <= std::numeric_limits<uint16_t>::max(),
				uint16_t,
				typename std::conditional<N <= std::numeric_limits<uint32_t>::max(),
					uint32_t,
					std::size_t
				>::type
			>::type
		>::type;

	static_assert(N > 0,
		"N must be greater than 0"
	);

	static_assert(sizeof(X) >= sizeof(idx_t),
		"N is too large for type X: sizeof(<the minimal underlying unsigned integral type for N>) must be not larger than sizeof(X)"
	);

	class Data {
	private:
/* Shame on Microsoft: the only one who does not support untagged unions */
#ifdef _MSC_VER
		union U {
#else
		union {
#endif
			X val;
			idx_t idx;
		} (*buf)[N];

		idx_t current;
		idx_t filled;

	public:
		Data() noexcept: buf{}, current{}, filled{} {
		}

		~Data() {
			if (buf) {
				std::free(buf);
			}
		}

		X *allocate() {
			if (!buf) {
				if (!(buf = reinterpret_cast<decltype(buf)>(std::malloc(sizeof *buf)))) {
					throw std::bad_alloc();
				}

				idx_t n{};

				for (auto &item: *buf) {
					item.idx = ++n;
				}

				current = 0;
				filled = 0;
			}

			idx_t const allocated = current;

			current = buf[0][current].idx;
			filled++;

			return &buf[0][allocated].val;
		}

		void deallocate(X *const p) noexcept {
			if (filled > 0) {
				idx_t const precurrent{static_cast<idx_t>(p - &buf[0]->val)};

				buf[0][precurrent].idx = current;
				current = precurrent;

				if (!--filled) {
					std::free(buf);
					buf = nullptr;
					current = 0;
				}
			}
		}

		bool can_allocate() const noexcept {
			return filled != N;
		}

		bool is_valid(X *const p) const noexcept {
			return p >= &buf[0]->val && p < &buf[1]->val;
		}
	};

	std::shared_ptr<Data> data;

public:
	SpecialAllocator() noexcept = default;

	SpecialAllocator(SpecialAllocator &&) noexcept = default;
	SpecialAllocator(SpecialAllocator const &) noexcept(noexcept(std::shared_ptr<Data>(data))) = default;

	SpecialAllocator &operator =(SpecialAllocator &&) = delete;
	SpecialAllocator &operator =(SpecialAllocator const &) = delete;

	X *allocate() {
		if (!data) {
			data = std::make_shared<Data>();
		}

		return data->allocate();
	}

	void deallocate(X *const p) noexcept {
		if (data) {
			data->deallocate(p);
		}
	}

	bool can_allocate() const noexcept {
		return !data || data->can_allocate();
	}

	bool is_valid(X *const p) const noexcept {
		return data && data->is_valid(p);
	}
};

} // namespace internal

template <typename X, std::size_t N>
class AllocatorN {
private:
	internal::SpecialAllocator<X, N> allocator;

public:
	using value_type = X;
	using size_type = std::size_t;

	AllocatorN() noexcept = default;

	AllocatorN(AllocatorN<X, N> &&) noexcept = default;
	AllocatorN(AllocatorN<X, N> const &) = default;

	template <typename Y, std::size_t M>
	AllocatorN &operator =(AllocatorN<Y, M> &&) = delete;

	template <typename Y, std::size_t M>
	AllocatorN &operator =(AllocatorN<Y, M> const &) = delete;

	template <typename Y, std::size_t M>
	AllocatorN(AllocatorN<Y, M> const &) = delete;

	template <typename Y, std::size_t M>
	AllocatorN(AllocatorN<Y, M> &&) = delete;

	template <typename Y, std::size_t M = N>
	struct rebind {
		using other = AllocatorN<Y, M>;
	};

	X *allocate(size_type const n) {
		if (n == 1 && allocator.can_allocate()) {
			return allocator.allocate();
		} else {
			void *const p{malloc(n * sizeof *allocate(n))};

			if (!p) {
				throw std::bad_alloc{};
			}

			return reinterpret_cast<X *>(p);
		}
	}

	void deallocate(X *const p, size_type const n) noexcept {
		if (n == 1 && allocator.is_valid(p)) {
			allocator.deallocate(p);
		} else {
			free(p);
		}
	}
};

template <typename X, std::size_t N, typename Y, std::size_t M>
static inline constexpr bool operator ==(AllocatorN<X, N> const &x, AllocatorN<Y, M> const &y) noexcept {
	return std::is_same<X, Y>::value && N == M && &x == &y;
}

template <typename X, std::size_t N, typename Y, std::size_t M>
static inline constexpr bool operator !=(AllocatorN<X, N> const &x, AllocatorN<Y, M> const &y) noexcept {
	return !operator ==(x, y);
}

} // namespace usr

#endif
