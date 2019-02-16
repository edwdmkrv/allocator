#ifndef __LIB_INCLUDED
#define __LIB_INCLUDED

#include <cstdlib>

#include <new>
#include <memory>
#include <utility>
#include <type_traits>

#include <iostream>

#include <version.hpp>

/* Version stuff
 */
static inline std::string ver() {
	return PROJECT_VERSION;
}

static inline unsigned major() {
	return PROJECT_VERSION_MAJOR;
}

static inline unsigned minor() {
	return PROJECT_VERSION_MINOR;
}

static inline unsigned patch() {
	return PROJECT_VERSION_PATCH;
}

/* Allocator stuff
 */
template <typename X, std::size_t N>
class AllocatorN {
private:

public:
	using value_type = X;
	using size_type = std::size_t;

	AllocatorN() noexcept {
	}

	template <typename Y, std::size_t M>
	AllocatorN(AllocatorN<Y, M> const &) = delete;

	template <typename Y, std::size_t M>
	AllocatorN(AllocatorN<Y, M> &&) {
	}

	template <typename Y, std::size_t M = N>
	struct rebind {
		using other = AllocatorN<Y, M>;
	};

	X *allocate(size_type const n) {
		void *const p{malloc(n * sizeof *allocate(n))};

		std::cout << __PRETTY_FUNCTION__ << ", n = " << n << ", p = " << p << std::endl;

		if (!p) {
			throw std::bad_cast{};
		}

		return reinterpret_cast<X *>(p);
	}

	void deallocate(X *const p, size_type const n) noexcept {
		std::cout << __PRETTY_FUNCTION__ << ", n = " << n << ", p = " << p << std::endl;
		free(p);
	}
};

template <typename X, std::size_t N, typename Y, std::size_t M>
static inline constexpr bool operator ==(AllocatorN<X, N> const &x, AllocatorN<Y, M> const &y) noexcept {
	return std::is_same<X, Y>::value && &x == &y;
}

template <typename X, std::size_t N, typename Y, std::size_t M>
static inline constexpr bool operator !=(AllocatorN<X, N> const &x, AllocatorN<Y, M> const &y) noexcept {
	return !operator ==(x, y);
}

/* Container stuff
 */
template <typename X, typename A = std::allocator<X>>
class ContainerExcecutive {
private:
	std::reference_wrapper<A> allocator;
	std::unique_ptr<X[], std::function<void(X *const)>> ptr;
	std::size_t asize;
	std::size_t dsize;

	decltype(auto) deleter() noexcept {
		return [this](X *const p) {
			while (dsize) {
				std::allocator_traits<A>::destroy(allocator, &p[--dsize]);
			}
		
			std::allocator_traits<A>::deallocate(allocator, p, asize);
		};
	}

protected:
	ContainerExcecutive(A &allocator, std::size_t const n = 0):
	allocator{allocator},
	ptr{n == 0 ? nullptr : std::allocator_traits<A>::allocate(allocator, n), deleter()},
	asize{n},
	dsize{} {
	}

	ContainerExcecutive<X, A>(ContainerExcecutive<X, A> &&vec) noexcept:
	allocator{std::move(vec.allocator)},
	ptr{std::move(vec.ptr)},
	asize{std::move(vec.asize)},
	dsize{std::move(vec.dsize)} {
		std::swap(ptr.get_deleter(), vec.ptr.get_deleter());
	}

	ContainerExcecutive<X, A>(ContainerExcecutive<X, A> const &vec):
	allocator{vec.allocator},
	ptr{nullptr, deleter()},
	asize{},
	dsize{} {
		reserve(vec.dsize);

		for (std::size_t n{}; n < asize; n++) {
			emplace_back(vec.ptr[n]);
		}
	}

	ContainerExcecutive<X, A> &operator =(ContainerExcecutive<X, A> &&vec) noexcept {
		std::swap(allocator, vec.allocator);
		std::swap(ptr, vec.ptr);
		std::swap(ptr.get_deleter(), vec.ptr.get_deleter());
		std::swap(asize, vec.asize);
		std::swap(dsize, vec.dsize);
		std::cout << __PRETTY_FUNCTION__ << ": " << this << ", vec = " << &vec << std::endl;
		return *this;
	}

	ContainerExcecutive<X, A> &operator =(ContainerExcecutive<X, A> const &vec) {
		ContainerExcecutive<X, A> replacement{vec.allocator, vec.dsize};

		for (std::size_t n{}; n < vec.dsize; n++) {
			replacement.emplace_back(vec.ptr[n]);
		}

		operator =(std::move(replacement));
		return *this;
	}

	void reserve(std::size_t const new_cap) {
		if (new_cap > asize) {
			ContainerExcecutive<X, A> vec{allocator, new_cap};

			for (; vec.dsize < dsize; vec.dsize++) {
				std::cout << __PRETTY_FUNCTION__ << ": " << this << ", n = " << vec.dsize << ", vec = " << &vec << std::endl;
				if constexpr (std::is_nothrow_move_constructible<X>::value) {
					std::allocator_traits<A>::construct(allocator, &vec.ptr[vec.dsize], std::move(ptr[vec.dsize]));
				} else {
					std::allocator_traits<A>::construct(allocator, &vec.ptr[vec.dsize], ptr[vec.dsize]);
				}
			}

			operator =(std::move(vec));
		}

	}

	template <typename... Y>
	X &emplace_back(Y &&... y) {
		reserve(dsize + 1);
		std::allocator_traits<A>::construct(allocator, &ptr[dsize], std::forward<Y>(y)...);
		return ptr[dsize++];
	}

	X *begin() noexcept {
		return &ptr[0];
	}

	X *end() noexcept {
		return &ptr[dsize];
	}

	bool empty() const noexcept {
		return !ptr;
	}

	std::size_t capacity() const noexcept {
		return asize;
	}

	std::size_t size() const noexcept {
		return dsize;
	}
};

template <typename X, typename A = std::allocator<X>>
class AllocatorWrapper {
protected:
	A allocator;
};

template <typename X, typename A = std::allocator<X>>
class Container: private AllocatorWrapper<X, A>, protected ContainerExcecutive<X, A> {
public:
	Container() noexcept : ContainerExcecutive<X, A>{AllocatorWrapper<X, A>::allocator} {
		std::cout << __PRETTY_FUNCTION__ << ": " << this << std::endl;
	}

	~Container() {
		std::cout << __PRETTY_FUNCTION__ << ": " << this << std::endl;
	}

	Container(Container const &) = default;
	Container(Container &&) = default;

	Container &operator =(Container &&) = default;
	Container &operator =(Container const &) = default;

	using ContainerExcecutive<X, A>::reserve;
	using ContainerExcecutive<X, A>::emplace_back;
	using ContainerExcecutive<X, A>::begin;
	using ContainerExcecutive<X, A>::end;
	using ContainerExcecutive<X, A>::empty;
	using ContainerExcecutive<X, A>::capacity;
	using ContainerExcecutive<X, A>::size;
};

#endif
