#include "utils/parsing.h"

namespace benchmark {

using std::string;

unsigned int NumArgs(string opts) {
  unsigned int ret = 0;
  // Set to true so we ignore spaces at the start.
  bool last_was_space = true;
  for (unsigned int i = 0; i < opts.size(); ++i) {
    if (opts[i] == ' ') {
      if (!last_was_space) {
        ++ret;
      }
      last_was_space = true;
    } else {
      last_was_space = false;
    }
  }
  // If the string didn't end in a space, then we are one arg short.
  if (!last_was_space) {
    ++ret;
  }
  return ret;
}

void ArgsToCArgs(char *args, char **argv) {
  // Set to true so we ignore spaces at the start.
  bool last_was_char = false;
  unsigned int argP = 0;
  for (unsigned int i = 0; args[i] != 0; ++i) {
    if (args[i] != ' ') {
      if (!last_was_char) {
        argv[argP++] = &args[i];
      }
      last_was_char = true;
    } else {
      args[i] = 0;
      last_was_char = false;
    }
  }
}

}  // namespace benchmark.
