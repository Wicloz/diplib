#include <iostream>
#include "diplib/library/types.h"

// Testing

int main() {
   bool error = false;

   dip::bin A = false;
   dip::bin B = true;
   if( !( A < B ) ) {
      error |= true;
   }
   if( !( B > A ) ) {
      error |= true;
   }
   if( !( A >= A ) ) {
      error |= true;
   }
   if( !( A <= B ) ) {
      error |= true;
   }
   if( !( A == A ) ) {
      error |= true;
   }
   if( !( A != B ) ) {
      error |= true;
   }
   if( !error ) {
      std::cout << "All tests passed\n";
   }
   return error;
}
