#include "application.h"
#include <iostream>
int main() {
  Application app;
  if (app.initialize()) {
    app.run();
  }
  app.shutdown();
  return 0;
}
