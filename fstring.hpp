/**
 * @file    fstring.hpp
 * @brief   A quick C++ formatting library
 * @version 0.1
 *
 * Copyright (c) 2022 Maysara Elshewehy (xeerx.com) (maysara.elshewehy@gmail.com)
 *
 * Distributed under the MIT License (MIT)
*/

#pragma once
#include <cstddef>      // std::size_t
#include <stdexcept>
#include <string>       // std::string
#include <cctype>       // isdigit
#include <utility>
#include <vector>       // std::vector
#include <algorithm>    // std::find


class fstring
{
private:
    // to save padding begin,end positions and value
    struct padd_vec
    {
        std::size_t begin, end, value;
    };

    std::string src;
    std::size_t lpos = 0; // last postion for variables() function
    std::vector<padd_vec> poss; // to remember positions before formatting in padding()
    char pad_char;

protected:
    // find paddings in string and handle it
    void padding() noexcept
    {
        // handle
        for (std::size_t pos = 0; pos < src.length(); pos++)
        {
            // find '%'
            if((pos = src.find('%', pos)) == src.npos) break;

            // now we got the position of first "%", so increase position to get next character
            auto tmp = pos ++;  // save begin position

            // what if the character "%" is last character in the string ?
            if(pos >= src.length()) break;

            // find '.' : for smart padding (padding - length of section)
            bool smart_padding = src[pos] == '.';

            // increase position to get the next character after "%."
            if(smart_padding) pos ++;

            decltype(pos) pos_start = pos;
            while (pos < src.length() && isdigit(src[pos])) { pos++; }
            uint8_t digits = pos - pos_start;
            uint32_t padd = 0;
            try
            {
                padd = std::stoi(src.substr(pos_start, digits));
            }
            catch(std::invalid_argument const& ex)
            {
                continue;
            }
            catch(std::out_of_range const& ex)
            {
                continue;
            }

            // if padding is zero so there is nothing to do so skip it !
            if(!padd) continue;

            // smart padding: save positions to apply after putting the variables value
            if(smart_padding)
            {
                std::size_t end;
                // find ".%" : the end of section
                if ((end = src.find(".%", pos)) == src.npos)
                {
                    src = "";
                    return;
                }
                poss.push_back(padd_vec{tmp, end - 4, padd});

                src.erase(end, 2);        // erase ".%"
                src.erase(tmp, digits + 2); // erase "%.n"
            }
            // normal padding: apply
            else
            {
                // erase %n
                src.erase(tmp, digits + 1);
                // add padding
                src.insert(tmp, padd, pad_char);

                // update position to avoid searching in padding area
                pos += padd - (digits + 1);
            }
        }
    }

    // apply smart padding
    void apply() noexcept
    {
        for (auto i = poss.begin(); i < poss.end(); i++)
        {
            // get length of section
            std::size_t length = i->end - i->begin;

            // if length < padding value so there are a padding char to add padding
            if(i->value > length)
            {
                // calc padding value: padding - length = the rest of padding
                std::size_t padd = i->value - length;

                // add padding
                src.insert(i->end, padd, pad_char);

                // now we updated the string value, so the length is updated
                // so  we need to update the upcoming positions with: +=padd
                for (auto t = i; t < poss.end(); t++) { t->begin += padd; t->end += padd; }
            }
        }
    }

    // inline helpers for variables()
    inline std::string tostr(std::string  ref) { return ref;                 }
    inline std::string tostr(const char * ref) { return std::string(ref);    }
    template<typename Arg>
    inline std::string tostr(Arg          ref) { return std::to_string(ref); }

    // replace "{}" by values
    template<typename T>
    void variables(T&& arg) noexcept
    {
        //  to avoid error(out of range exception)
        if(lpos == src.npos) return;

        // find "{}"
        if ((lpos = src.find("{}", lpos)) == src.npos) return;

        // store arg value as string
        auto val = tostr(std::forward<T>(arg));
        auto val_len = val.length();

        // save
        src.erase(lpos, 2);    // erase "{}"
        src.insert(lpos, val); // add arg value in position of "{}"

        // now we updated the string value, so the length is updated
        // so  we need to update the upcoming positions with: +=val.length()
        for (auto i = poss.begin(); i < poss.end(); i++)
        {
            // skip values smaller than the lpos
            if(i->end <= lpos) continue;

            // increase
            i->end   += val_len;

            // we cleared the "{}" characters so we need to go back 2 characters if possible
            if(i->end   > 2) i->end   -=2;

            // begin of non-first
            if(i != poss.begin())
            {
                i->begin += val_len;
                if(i->begin > 2) i->begin -=2;
            }
        }
    }

public:
    // constructor
    template<typename... T>
    fstring(std::string _src, T&& ...args) : fstring(' ', _src, args...)
    {
    }

    template<typename... T>
    fstring(char _pad_char, std::string _src, T&& ...args) :
        pad_char(_pad_char),
        src{std::move(_src)}
    {
        // performance improvement, see: https://cplusplus.com/reference/vector/vector/reserve/
        poss.reserve(  5);
        src .reserve(250);

        padding();
        ( variables(std::forward<T>(args)) , ...);
        apply  ();
    }

    // get result
    std::string& get(){ return src; }
};
