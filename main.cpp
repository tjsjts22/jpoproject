#include <iostream>
using namespace std;
#include <fstream>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}
/*
void build(auto input, nlohmann::json &jsonData, nlohmann::json &jsonDatabase){
}
*/

void fetchAndSaveData(const string& url, const string& filename) {
    CURL* curl;
    CURLcode result;
    string responseString;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        result = curl_easy_perform(curl);

        if (result != CURLE_OK) {
            cerr << "cURL Error: " << curl_easy_strerror(result) << endl;
        } else {
            // Convert response to JSON and save it
            try {
                nlohmann::json jsonData = nlohmann::json::parse(responseString);

                ofstream file(filename);
                if (file.is_open()) {
                    file << jsonData.dump(4);  // Pretty print with indentation
                    file.close();
                    cout << "Data saved to " << filename << endl;
                } else {
                    cerr << "Error opening file: " << filename << endl;
                }
            } catch (nlohmann::json::parse_error& e) {
                cerr << "JSON Parsing Error: " << e.what() << endl;
            }
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void init(){
    //opening file in cpp (GIVEN JSON)
    ifstream file("findAllmine.json");
    if(file.is_open()){
        cout << "File with name of base file exists no need to download new one.\n";
        
        char answer;
        cout << "Do you wish to download database? y/n ";
        cin >> answer;
        cout << "\n";

        if(answer=='y'){
            fetchAndSaveData("https://api.gios.gov.pl/pjp-api/rest/station/findAll", "findAllmine.json");
        }
    }else{
    fetchAndSaveData("https://api.gios.gov.pl/pjp-api/rest/station/findAll", "findAllmine.json");
    }

    //creating object called jsonData and inputing into it our json data
    nlohmann::json jsonData;
    file >> jsonData;
    file.close();

    nlohmann::json jsonDatabase; //creating json file with citys names
    // Accessing specific data
    

    for (const auto& station : jsonData) {
        if (station.contains("city")){
            string cityName = station["city"]["name"].get<string>();
            int id = station["id"].get<int>();
            string provinceName = station["city"]["commune"]["provinceName"].get<string>();
            nlohmann::json newEntry;

            newEntry["id"] = id;
            newEntry["provinceName"]= provinceName;
            newEntry["cityName"] = cityName;
            

            jsonDatabase.push_back(newEntry);
        }//jsonDatabase.push_back({})
        // data is read as 2d array with "indexes" city and name which outputs citys names
        // additionally it writes to file called data which from now on will contain 
    }

    ofstream outFile("database.json");
    if (!outFile.is_open()) {
        cerr << "Could not open database.json for writing!" << endl;
    }
    // Writing the JSON data to the file
    outFile << jsonDatabase.dump(4);  // Indentation of 4 for better readability
    outFile.close();  // Close the file
}

void updateData(){
    //void fetchAndSaveData(const string& url, const string& filename)
    ifstream dataFile("database.json");
    string stationID;
    cin >> stationID;
    fetchAndSaveData(("https://api.gios.gov.pl/pjp-api/rest/aqindex/getIndex/"+stationID), "stationData.json");
}

void search(){
    string input;
    cout << "input name of the city you wish to select";
    ifstream dataFile("database.json");
    if(!dataFile.is_open()){
        cerr << "DATABASE ERROR OH NiOOOOOOOOOOO!";
    }

    nlohmann::json dataFromDatabase;
    dataFile >> dataFromDatabase;

    
    while(true){
        cout << "input name of the city you wish to select :";
        cin >> input; cout << "\n";
        for (const auto& city : dataFromDatabase){
                if (city["cityName"]==input){
                    cout << city["id"] << " " << city["cityName"] << " " << city["provinceName"] << "\n";
                }
        }
        char answer;
        cout << "is that correct? y/n ";
        cin >> answer;
        cout << "\n";
        if(answer=='y') break;
    }    
    dataFile.close();
}



int main(){
    
    init();

    while(true){
        cout << "1. Search\n2. Read Station data\n10. exit\n"; 
        int function;
        cin >> function;
        switch(function){
            case 1:{
                search();
                break;
            }
            case 2:{
                updateData();
                break;
            }
            case 10:{
                return 0;
                break;
            }
        }
}   
}