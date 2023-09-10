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

#include <sstream>
#include <fstream>
#include <sys/sysinfo.h>

#include "memory.h"
#include "conversions.h"

void mem_status( MemoryStatus & status )
{
  std::string line;
  std::string substr;
  size_t substr_start;
  size_t substr_len;

  unsigned int total_mem;
  unsigned int used_mem;

  /* Read /proc/meminfo to find the amount of total and used RAM, assuming a
   * recent kernel (3.14+) that calculates the amount of available RAM for us.
   *
   * https://github.com/torvalds/linux/commit/34e431b0a
   */

  std::ifstream memory_info("/proc/meminfo");

  while( std::getline( memory_info, line ) )
  {
    substr_start = 0;
    substr_len = line.find_first_of( ':' );
    substr = line.substr( substr_start, substr_len );
    substr_start = line.find_first_not_of( " ", substr_len + 1 );
    substr_len = line.find_first_of( 'k' ) - substr_start;
    if( substr.compare( "MemTotal" ) == 0 )
    {
      // get total memory
      total_mem = stoi( line.substr( substr_start, substr_len ) );
    }
    else if( substr.compare( "MemAvailable" ) == 0 ) {
      used_mem = total_mem - stoi( line.substr( substr_start, substr_len ) );
    }
  }

  // we want megabytes on output, but since the values already come as
  // kilobytes we need to divide them by 1024 only once, thus we use
  // KILOBYTES
  status.used_mem = convert_unit(static_cast< float >( used_mem ), MEGABYTES, KILOBYTES);
  status.total_mem = convert_unit(static_cast< float >( total_mem ), MEGABYTES, KILOBYTES);
}
