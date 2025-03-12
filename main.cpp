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
    file.close();

    nlohmann::json jsonDatabase; //creating json file with citys names
    // Accessing specific data
    //Search tool
    string input;
    cin >> input;

    for (const auto& station : jsonData) {
        if (station["city"]["name"]==input) {
            string cityName = station["city"]["name"].get<string>();
            int id = station["id"].get<int>();
            string provinceName = station["city"]["commune"]["provinceName"].get<string>();
            nlohmann::json newEntry;

            newEntry["id"] = id;
            newEntry["provinceName"]= provinceName;
            newEntry["cityName"] = cityName;
            

            jsonDatabase.push_back(newEntry);
            //jsonDatabase.push_back({})
        }// data is read as 2d array with "indexes" city and name which outputs citys names
        // additionally it writes to file called data which from now on will contain 
    }
    ofstream outFile("database.json");
    if (!outFile.is_open()) {
        cerr << "Could not open database.json for writing!" << endl;
        return 1;
    }
    // Writing the JSON data to the file
    outFile << jsonDatabase.dump(4);  // Indentation of 4 for better readability
    outFile.close();  // Close the file
    
    if (dbFile.is_open()) {
        dbFile >> jsonDatabase;  // If file exists, load existing data
        dbFile.close();
    } else {
        cerr << "Could not open database.json for reading, creating a new one." << endl;
    }
    
}