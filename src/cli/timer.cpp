/*
* (C) 2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "timer.h"

#include <botan/internal/os_utils.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace Botan_CLI {

namespace {

std::string format_timer_name(std::string_view name, std::string_view provider) {
   if(provider.empty() || provider == "base") {
      return std::string(name);
   }

   std::ostringstream out;
   out << name << " [" << provider << "]";
   return out.str();
}

}  // namespace

Timer::Timer(std::string_view name,
             std::string_view provider,
             std::string_view doing,
             uint64_t event_mult,
             size_t buf_size,
             double clock_cycle_ratio,
             uint64_t clock_speed) :
      m_name(format_timer_name(name, provider)),
      m_doing(doing),
      m_buf_size(buf_size),
      m_event_mult(event_mult),
      m_clock_cycle_ratio(clock_cycle_ratio),
      m_clock_speed(clock_speed) {}

void Timer::start() {
   stop();
   m_timer_start = Botan::OS::get_system_timestamp_ns();
   m_cpu_cycles_start = Botan::OS::get_cpu_cycle_counter();
}

void Timer::stop() {
   if(m_timer_start) {
      const uint64_t now = Botan::OS::get_system_timestamp_ns();

      if(now > m_timer_start) {
         m_time_used += (now - m_timer_start);
      }

      if(m_cpu_cycles_start != 0) {
         const uint64_t cycles_taken = Botan::OS::get_cpu_cycle_counter() - m_cpu_cycles_start;
         if(cycles_taken > 0) {
            m_cpu_cycles_used += static_cast<size_t>(cycles_taken * m_clock_cycle_ratio);
         }
      }

      m_timer_start = 0;
      ++m_event_count;
   }
}

bool Timer::operator<(const Timer& other) const {
   if(this->doing() != other.doing()) {
      return (this->doing() < other.doing());
   }

   return (this->get_name() < other.get_name());
}

std::string Timer::to_string() const {
   if(!m_custom_msg.empty()) {
      return m_custom_msg;
   } else if(this->buf_size() == 0) {
      return result_string_ops();
   } else {
      return result_string_bps();
   }
}

std::string Timer::result_string_bps() const {
   const size_t MiB = 1024 * 1024;

   const double MiB_total = static_cast<double>(events()) / MiB;
   const double MiB_per_sec = MiB_total / seconds();

   std::ostringstream oss;
   oss << get_name();

   if(!doing().empty()) {
      oss << " " << doing();
   }

   if(buf_size() > 0) {
      oss << " buffer size " << buf_size() << " bytes:";
   }

   if(events() == 0) {
      oss << " "
          << "N/A";
   } else {
      oss << " " << std::fixed << std::setprecision(3) << MiB_per_sec << " MiB/sec";
   }

   if(cycles_consumed() != 0) {
      const double cycles_per_byte = static_cast<double>(cycles_consumed()) / events();
      oss << " " << std::fixed << std::setprecision(2) << cycles_per_byte << " cycles/byte";
   }

   oss << " (" << MiB_total << " MiB in " << milliseconds() << " ms)\n";

   return oss.str();
}

std::string Timer::result_string_ops() const {
   std::ostringstream oss;

   oss << get_name() << " ";

   if(events() == 0) {
      oss << "no events\n";
   } else {
      oss << static_cast<uint64_t>(events_per_second()) << ' ' << doing() << "/sec; " << std::setprecision(2)
          << std::fixed << ms_per_event() << " ms/op";

      if(cycles_consumed() != 0) {
         const double cycles_per_op = static_cast<double>(cycles_consumed()) / events();
         const int precision = (cycles_per_op < 10000) ? 2 : 0;
         oss << " " << std::fixed << std::setprecision(precision) << cycles_per_op << " cycles/op";
      }

      oss << " (" << events() << " " << (events() == 1 ? "op" : "ops") << " in " << milliseconds() << " ms)\n";
   }

   return oss.str();
}

}  // namespace Botan_CLI
