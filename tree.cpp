// Copyright (c) 2018 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


// TODO:
//  * clean up the mess in code
//  * clean up after the cleanup
//  * add falling snow or glowing stars
//  * more modes for lamps

#include <cstdlib>
#include <chrono>
#include <thread>
#include <iostream>
#include <random>
#include <iterator>
#include <fstream>
#include <algorithm>
#include <atomic>

enum class color_t : unsigned char {};
constexpr color_t operator"" _c(unsigned long long int v) { return color_t(v); }

std::ostream& operator<< (std::ostream& os, color_t val) {
    return os << "\033[38;5;" << static_cast<int>(val) << 'm';
}


struct green_state {
  color_t operator()() const {
    return *(dark >= std::size(greens) ? std::end(greens) - 1 : std::begin(greens) + dark);
  }

  void operator++() { ++dark; }
  void reset() { dark = 0; }

private:
  constexpr static color_t greens[] = { 22_c, 22_c, 28_c, 28_c, 34_c };
  int dark = 0;
};

struct lamps {
  void operator()(char c) {
    if (mode % max_state == 0)
      new_color();
    std::cout << col << c;
  }

  void end_cycle() {
    if (mode % max_state == 1) {
      new_color();
    } else if (mode % max_state == 2) {
      auto i = static_cast<int>(col);
      ++i;

      i %= 226;
      i = std::clamp(i, 154, 226);

      //i %= 202;
      //i = std::clamp(i, 196, 202);
      col = static_cast<color_t>(i);
    }
  }

  void change_mode() {
    ++mode;
  }
private:
  std::atomic<int> mode{0};
  static constexpr int max_state = 3;

  void new_color() {
    std::sample(std::begin(pretty_colors), std::end(pretty_colors), &col, 1, rand);
  }

  std::mt19937 rand{std::random_device{}()};
  color_t col = 0_c;

  // https://misc.flogisoft.com/bash/tip_colors_and_formatting
  static constexpr color_t pretty_colors[] = {
    1_c, 9_c, 11_c, 15_c, 45_c, 87_c, 118_c, 154_c, 155_c, 165_c, 193_c, 196_c, 198_c,208_c, 226_c, 15_c
  };
};

int main(int args, const char** argv) {
  std::string filename = "xtree.txt";
  if (args > 1) {
    filename = argv[1];
  }

  using namespace std::literals::chrono_literals;

  std::string tree;
  lamps lamp;

  std::thread t([&lamp]() {
    char c;
    while (std::cin >> c) {
      lamp.change_mode();
    }
  });

  for (;;) {
    std::system("clear");
    std::ifstream ifs{filename.c_str()};
    std::getline(ifs, tree, '\0');

    green_state g;
    auto it = tree.begin();
    auto end = tree.end();
    auto prev_width = 0;
    for (; it != end; ++it) {
      const auto c = *it;
      switch (c) {
      case '*': [[fall_through]]
      case ' ':
        std::cout << g() << c;
        break;

      case 'o': [[fall_through]]
      case 'O':
        lamp(c);
        break;

      case '#':
        std::cout << 94_c << c;
        break;

      case '\n': {
          auto nl1 = std::find(it, end, '\n');
          if (nl1 == end) nl1 = it;
          const auto nl2 = std::find(nl1 + 1, end, '\n');
          const auto new_width = std::count(nl1, nl2, '*') + std::count(nl1, nl2, 'o') + std::count(nl1, nl2, 'O');
          if (prev_width < new_width) {
            ++g;
          } else {
            g.reset();
          }
          prev_width = new_width;
        }
        [[fall_through]]

      default:
        std::cout << 0_c << c;
      }
    }

    std::cout << 0_c << std::endl;
    lamp.end_cycle();
    std::this_thread::sleep_for(400ms);
  }

  t.join();
}
