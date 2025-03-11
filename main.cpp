#include <iostream>
using namespace std;
#include <fstream>
#include <nlohmann/json.hpp>

int main(){
    //opening file in cpp (GIVEN JSON)
    ifstream file("findAll.json");
    if(!file.is_open()){
        cerr << "Could not open the file!";
        return 1;
    }
    //creating object called jsonData and inputing into it our json data
    nlohmann::json jsonData;
    file >> jsonData;



    // Accessing specific data
    for (const auto& station : jsonData) {
        if (station.contains("city")) {
            cout << "City Name: " << station["city"]["name"].get<string>() << endl;
        }// data is read as 2d array with "indexes" city and name which outputs citys names
        
    }
}