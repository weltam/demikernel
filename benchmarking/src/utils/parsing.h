#ifndef UTILS_PARSING_H_
#define UTILS_PARSING_H_

#include <string>

namespace benchmark {

unsigned int NumArgs(std::string opts);
void ArgsToCArgs(char *args, char **argv);

}  // namespace benchmark.
#endif  // UTILS_PARSING_H_
