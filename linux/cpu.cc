/* vim: tabstop=2 shiftwidth=2 expandtab textwidth=80 linebreak wrap
 *
 * Copyright 2012 Matthew McCormick
 * Copyright 2015 Pawel 'l0ner' Soltys
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <filesystem>
#include <fstream>
#include <optional>
#include <unistd.h> // usleep

#include "cpu.h"
#include "luts.h"

uint32_t get_cpu_count()
{
  return sysconf( _SC_NPROCESSORS_ONLN );
}

float cpu_percentage( unsigned cpu_usage_delay )
{
  std::string line;
  size_t substr_start = 0;
  size_t substr_len;

  // cpu stats
  // user, nice, system, idle
  // in that order
  unsigned long long stats[CP_STATES];

  std::ifstream stat_file( "/proc/stat" );
  getline( stat_file, line );
  stat_file.close();

  // skip "cpu"
  substr_len = line.find_first_of( " ", 3 );
  // parse cpu line
  for( unsigned i=0; i < 4; i++ )
  {
    substr_start = line.find_first_not_of( " ", substr_len );
    substr_len   = line.find_first_of( " ", substr_start );
    stats[i] = std::stoll( line.substr( substr_start, substr_len ) );
  }

  usleep( cpu_usage_delay );

  stat_file.open( "/proc/stat" );
  getline( stat_file, line );
  stat_file.close();

  // skip "cpu"
  substr_len = line.find_first_of( " ", 3 );
  // parse cpu line
  for( unsigned i=0; i < 4; i++ )
  {
    substr_start = line.find_first_not_of( " ", substr_len );
    substr_len   = line.find_first_of    ( " ", substr_start );
    stats[i] = std::stoll( line.substr( substr_start, substr_len ) ) - stats[i];
  }

  return static_cast<float>(
    stats[CP_USER] + stats[CP_NICE] + stats[CP_SYS]
    ) / static_cast<float>(
        stats[CP_USER] + stats[CP_NICE] + stats[CP_SYS] + stats[CP_IDLE]
    ) * 100.0;
}

// Obtain cpu core temperatures from sysfs.
// TODO: only tested with a single-socket Intel chip, Kernel 6.2
float cpu_temp_c(CPU_TEMP_MODE mode) {
  static constexpr float MIN_TEMP = -273.15f;
  static const std::filesystem::path hwmon_base_dir("/sys/devices/platform/coretemp.0/hwmon");

  if (!std::filesystem::exists(hwmon_base_dir)) {
    return MIN_TEMP;
  }

  std::optional<std::filesystem::path> hwmonX_dir;
  for (const auto& dir_entry : std::filesystem::directory_iterator(hwmon_base_dir)) {
    if (dir_entry.is_directory() && dir_entry.path().filename().string().starts_with("hwmon")) {
      hwmonX_dir = dir_entry.path();
      break;
    }
  }

  if (!hwmonX_dir.has_value()) {
    return MIN_TEMP;
  }

  float max_temp = MIN_TEMP;
  float sum_temp = 0;
  int zone_count = 0;

  std::string label;
  unsigned milli_c;
  // easy, but perhaps not the best way to do this
  for (unsigned i = 1; i < 9999; ++i) {
    const std::filesystem::path label_path =
        hwmonX_dir.value() / ("temp" + std::to_string(i) + "_label");
    const std::filesystem::path input_path =
        hwmonX_dir.value() / ("temp" + std::to_string(i) + "_input");

    if (!std::filesystem::exists(label_path)) {
      break;
    }

    std::ifstream label_file(label_path);
    label_file >> label;

    std::ifstream input_file(input_path);
    input_file >> milli_c;

    if (label.starts_with("Package")) {
      max_temp = static_cast<float>(milli_c);
    } else {
      sum_temp += static_cast<float>(milli_c);
      ++zone_count;
    }
  }

  if (mode == CPU_TEMP_MODE_MAX) {
    return max_temp * 0.001f;
  } else {
    return sum_temp / static_cast<float>(zone_count) * 0.001f;
  }
}
