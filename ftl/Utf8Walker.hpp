/*
 * Utf8Walker.hpp -- UTF8 character walker
 *
 * Copyright (c) 2007-2010, Frank Mertens
 *
 * See ../COPYING for the license.
 */
#ifndef FTL_UTF8WALKER_HPP
#define FTL_UTF8WALKER_HPP

#include <stdint.h>

namespace ftl
{

class String;

/** \brief Iterating UTF8 encoded strings
  *
  * The Utf8Walker allows iterating UTF8 strings efficiently.
  * Use with care! Strings must be zero terminated and therefore can't contain
  * zeros itself. Behavior is undefined for strings encoded with a different
  * encoding than UTF8.
  */
class Utf8Walker
{
public:
	// Initialize new iterator
	// \arg data utf8 encoded string
	// \arg pos position in string
	Utf8Walker(const char* data = 0, const char* pos = 0)
		: s_(reinterpret_cast<const uint8_t*>(data)),
		  p_(reinterpret_cast<const uint8_t*>((pos == 0) ? data : pos)),
		  valid_(p_ != 0)
	{
		if (valid_) valid_ = (*p_ != 0);
	}
	
	// prefix increment
	inline Utf8Walker& operator++() {
		check(valid());
		// step forwards consuming all prefix bits of first byte
		uint8_t ch = *p_;
		++p_;
		if (0x7F < ch) {
			--p_;
			do {
				++p_;
				ch <<= 1;
			} while (0x7F < ch);
		}
		if (*p_ == 0)
			valid_ = false;
		return *this;
	}
	
	// prefix decrement
	inline Utf8Walker& operator--() {
		check(s_ <= p_);
		// step backwards while code prefix equals (10)2
		if (p_ <= s_) valid_ = false;
		else {
			do {
				--p_;
			} while ((*p_ & 0xC0) == 0x80);
			if (!valid_)
				valid_ = true;
		}
		return *this;
	}
	
	// postfix increment
	inline Utf8Walker operator++(int) {
		Utf8Walker it = *this;
		++(*this);
		return it;
	}
	
	// postfix decrement
	inline Utf8Walker operator--(int) {
		Utf8Walker it = *this;
		--(*this);
		return it;
	}
	
	inline Utf8Walker& operator+=(int n) {
		while (n > 0) { ++(*this); --n; }
		while (n < 0) { --(*this); ++n; }
		return *this;
	}
	
	inline Utf8Walker& operator-=(int n) {
		while (n > 0) { --(*this); --n; }
		while (n < 0) { ++(*this); ++n; }
		return *this;
	}
	
	inline bool valid() const { return valid_; }
	
	inline Utf8Walker operator+(int delta) const {
		Utf8Walker it = *this;
		return it += delta;
	}
	inline Utf8Walker operator-(int delta) const {
		Utf8Walker it = *this;
		return it -= delta;
	}
	
private:
	friend class String;
	
	// decode unicode point
	inline uint32_t getChar() const {
		check(valid());
		// merging extra bytes by incrementally reducing the code prefix of the first byte
		// prefix bits => extra bytes: (110)2 => 1 eb, (1110)2 => 2 eb, (11110)2 => 3 eb
		uint32_t ch = *p_;
		if (0x7F < ch) {
			const uint8_t* p = p_;
			uint32_t bit = 0x80;
			ch ^= bit; // clear first prefix bit
			bit >>= 1; // take next prefix bit
			while ((ch & bit) != 0) { // yet another prefix bit?
				ch ^= bit; // clear prefix bit of first byte
				ch <<= 6; // make space
				bit <<= 5; // take next prefix bit
				ch |= (*(++p)) & 0x3F; // merge code bits
			}
		}
		return ch;
	}
	
	inline const char* data() const { return reinterpret_cast<const char*>(s_); }
	inline const char* pos() const { return reinterpret_cast<const char*>(p_); }
	
	const uint8_t* s_; // string
	const uint8_t* p_; // position in string
	bool valid_;
};

} // namespace ftl

#endif // FTL_UTF8WALKER_HPP
