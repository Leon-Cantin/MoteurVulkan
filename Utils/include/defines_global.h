#pragma once

#define IN_USE 9
#define NOT_IN_USE ( -9 )
#define USE_IF( x ) ( ( x ) ? 9 : -9 )
#define USING( x ) ( 9 / ( x ) == 1 )