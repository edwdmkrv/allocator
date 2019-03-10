#ifndef __CONTAINER_INCLUDED
#define __CONTAINER_INCLUDED

#include <memory>
#include <utility>
#include <limits>
#include <type_traits>

/* Container stuff
 */
template <typename X, typename A = std::allocator<X>>
class ContainerExecutive {
private:
	static_assert(std::is_nothrow_destructible<X>::value,
		"Types with destructors that can throw exceptions are not supported"
	);

	A &allocator;
	X *ptr;
	std::size_t asize;
	std::size_t dsize;

	static constexpr std::size_t min_size(std::size_t const numerator, std::size_t const denominator, std::size_t const val = 0) {
		return (val * numerator + denominator / 2) / denominator > val ? val : min_size(numerator, denominator, val + 1);
	}

	std::size_t new_size() const {
		enum: std::size_t {
			size_t_max = std::numeric_limits<std::size_t>::max(),
			numerator = 21,
			denominator = 13
		}; // 21 / 13 ~ (sqrt(5) + 1) / 2 (golden section)

		if (asize == size_t_max) {
			throw std::bad_alloc{};
		}

		return
			asize == 0 ?
				min_size(numerator, denominator) :
			asize < (size_t_max - denominator / 2) / numerator ?
				(asize * numerator + denominator / 2) / denominator :
			asize < size_t_max / numerator * denominator ?
				asize / denominator * numerator :
				size_t_max;
	}

	void check_bounds(std::size_t const pos) const {
		if (pos >= dsize) {
			throw std::out_of_range{"Index is out of range"};
		}
	}

protected:
	explicit ContainerExecutive(A &allocator) noexcept:
	allocator{allocator},
	ptr{},
	asize{},
	dsize{} {
	}

	ContainerExecutive(A &allocator, std::size_t const asize):
	allocator{allocator},
	ptr{std::allocator_traits<A>::allocate(allocator, asize)},
	asize{asize},
	dsize{} {
	}

	~ContainerExecutive() {
		while (dsize) {
			std::allocator_traits<A>::destroy(allocator, ptr + --dsize);
		}

		std::allocator_traits<A>::deallocate(allocator, ptr, asize);
	}

	void reserve(std::size_t const new_cap) {
		if (new_cap > asize) {
			if (asize) {
				ContainerExecutive<X, A> vec{allocator, new_cap};

				for (; vec.dsize < dsize; vec.dsize++) {
					std::allocator_traits<A>::construct(allocator, &vec.ptr[vec.dsize], std::move_if_noexcept(ptr[vec.dsize]));
				}

				std::swap(ptr, vec.ptr);
				std::swap(asize, vec.asize);
				std::swap(dsize, vec.dsize);
			} else {
				ptr = std::allocator_traits<A>::allocate(allocator, new_cap);
				asize = new_cap;
			}
		}

	}

	template <typename... Y>
	X &emplace_back(Y &&... y) {
		if (dsize == asize) {
			reserve(new_size());
		}

		std::allocator_traits<A>::construct(allocator, ptr + dsize, std::forward<Y>(y)...);
		return ptr[dsize++];
	}

	X &at(std::size_t const pos) {
		check_bounds(pos);
		return ptr[pos];
	}

	X const &at(std::size_t const pos) const {
		check_bounds(pos);
		return ptr[pos];
	}

	X *begin() noexcept {
		return ptr;
	}

	X *end() noexcept {
		return ptr + dsize;
	}

	X const *begin() const noexcept {
		return ptr;
	}

	X const *end() const noexcept {
		return ptr + dsize;
	}

	bool empty() const noexcept {
		return !dsize;
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
class Container: private AllocatorWrapper<X, A>, protected ContainerExecutive<X, A> {
public:
	Container() noexcept(noexcept(A{})) : ContainerExecutive<X, A>{AllocatorWrapper<X, A>::allocator} {
	}

	using ContainerExecutive<X, A>::reserve;
	using ContainerExecutive<X, A>::emplace_back;
	using ContainerExecutive<X, A>::at;
	using ContainerExecutive<X, A>::begin;
	using ContainerExecutive<X, A>::end;
	using ContainerExecutive<X, A>::empty;
	using ContainerExecutive<X, A>::capacity;
	using ContainerExecutive<X, A>::size;
};

#endif
