#pragma once

// do not include this from another header file or things will break

#define REQUIRE(COND, LABEL) if (!(cond)) { goto LABEL; }