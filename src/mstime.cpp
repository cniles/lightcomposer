#include <chrono>

long millis_since_epoch() {
  auto t_s = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(t_s).count();
}

long next_half_second() {
  long m_s = millis_since_epoch();  
  return m_s + (1000 - (m_s % 500));
}

