#ifndef STATE
#define STATE
//set initial state

struct State {   
    float humidity;
    float pressure;
    double pressure64;
    float temp;   String filename;
    bool reload;
    String json;
    bool ota;
    float dew () {
      return  temp - ((100 - humidity) / 5.);
    }

  };

  State state = {0.00, 0.00, 0.00, 0.00,false,"",false};
#endif 
