#include "includes/main.h"

int main(int argc, char ** argv){
  
    Instancia * instancia = NULL;

    init(&instancia, argv);
    run(&instancia);
    end(&instancia);

    return 0;
}