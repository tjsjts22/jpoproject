/**
 * @file main.cpp
 * @brief Main application file for the JPO project.
 *
 * This application uses wxWidgets to create a GUI for fetching and displaying sensor data.
 * It uses libcurl for HTTP requests and nlohmann::json for JSON parsing.
 */

 #include <wx/wx.h>
 #include <fstream>
 #include <nlohmann/json.hpp>
 #include <curl/curl.h>
 #include <string>
 #include <vector>
 #include <iostream>
 #include <wx/dc.h>
 #include <wx/bitmap.h>
 #include <numeric> // For accumulate
 #include <wx/datectrl.h>
 #include <wx/datetime.h> // For date ranges
 #include <thread>
 
 using namespace std;
 
 /**
  * @class GraphPanel
  * @brief Custom wxPanel to draw a graph based on sensor data.
  *
  * The GraphPanel class takes a vector of JSON objects representing sensor data.
  * It draws a graph with grid lines, axes, data points, and labels for min, max, average, and trend.
  * graph min and max values are based on min and max values that are marked and display with coresponding date
  */
 class GraphPanel : public wxPanel {
 public:
     /**
      * @brief Constructs a GraphPanel.
      * @param parent Pointer to the parent wxWindow.
      * @param data A vector of JSON objects with sensor data.
      */
     GraphPanel(wxWindow* parent, const vector<nlohmann::json>& data)
         : wxPanel(parent), sensorData(data) {
         Bind(wxEVT_PAINT, &GraphPanel::OnPaint, this);
     }
 
 private:
     vector<nlohmann::json> sensorData; ///< Sensor data in JSON format
 
     /**
      * @brief Paint event handler.
      *
      * Called by wxWidgets when the panel needs to be repainted.
      * Creates a thread to draw the graph.
      *
      * @param event The paint event.
      */
     void OnPaint(wxPaintEvent& event) {
         wxPaintDC dc(this);
         // Draw the graph in a separate thread to avoid blocking the main thread.
         thread t(&GraphPanel::DrawGraph, this, ref(dc));
         t.join();
     }
 
     /**
      * @brief Draws the sensor data graph.
      *
      * Draws grid lines, axes, sensor data as connected lines and circles,
      * and annotates the graph with min, max, average values and a trend.
      *
      * @param dc The device context for drawing.
      */
     void DrawGraph(wxDC& dc) {
         // Define margins for axis labels and grid boundaries.
         const int leftMargin = 60;
         const int rightMargin = 50;
         const int topMargin = 50;
         const int bottomMargin = 200;
         
         int panelWidth = GetSize().GetWidth();
         int panelHeight = GetSize().GetHeight();
     
         // Use a dotted pen for grid lines.
         wxPen gridPen(wxColour(200, 200, 200), 1, wxPENSTYLE_DOT);
         
         // Draw horizontal grid lines.
         int horizontalDivisions = 10;
         int gridStepY = (panelHeight - topMargin - bottomMargin) / horizontalDivisions;
         for (int i = 0; i <= horizontalDivisions; ++i) {
             int y = topMargin + i * gridStepY;
             dc.SetPen(gridPen);
             dc.DrawLine(leftMargin, y, panelWidth - rightMargin, y);
         }
     
         // Set the font for labels.
         dc.SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
     
         // Extract sensor values and corresponding dates from the JSON data.
         vector<double> values;
         vector<string> dates;
         for (const auto& sensor : sensorData) {
             for (const auto& dataEntry : sensor["values"]) {
                 // Process only valid data entries.
                 if (!dataEntry.contains("value") || dataEntry["value"].is_null())
                     continue;
                 values.push_back(dataEntry["value"].get<double>());
                 dates.push_back(dataEntry["date"].get<string>());
             }
         }
         
         // Reverse arrays so that the earliest date is drawn on the left.
         reverse(values.begin(), values.end());
         reverse(dates.begin(), dates.end());
         
         if (values.empty())
             return;
     
         // Determine min and max values for scaling.
         double maxValue = *max_element(values.begin(), values.end());
         double minValue = *min_element(values.begin(), values.end());
         double yRange = maxValue - minValue;
         if (yRange == 0)
             yRange = 1; // Prevent division by zero.
     
         // Calculate scaling factors for converting data points to screen coordinates.
         double scaleX = (panelWidth - leftMargin - rightMargin) / static_cast<double>(values.size() - 1);
         double scaleY = (panelHeight - topMargin - bottomMargin) / yRange;
         
         // Determine label interval so vertical grid lines match date labels.
         int verticalDivisions = 10;
         size_t labelInterval = (values.size() / verticalDivisions) + 1;
         
         // Draw vertical grid lines at label positions.
         for (size_t i = 0; i < values.size(); i += labelInterval) {
             double x = leftMargin + i * scaleX;
             dc.SetPen(gridPen);
             dc.DrawLine(x, topMargin, x, panelHeight - bottomMargin);
         }
     
         // Draw axes over the grid.
         dc.SetPen(*wxBLACK_PEN);
         dc.DrawLine(leftMargin, panelHeight - bottomMargin, panelWidth - rightMargin, panelHeight - bottomMargin); // X-axis
         dc.DrawLine(leftMargin, panelHeight - bottomMargin, leftMargin, topMargin); // Y-axis
     
         // Connect data points with lines.
         for (size_t i = 1; i < values.size(); ++i) {
             double x1 = leftMargin + (i - 1) * scaleX;
             double y1 = panelHeight - bottomMargin - (values[i - 1] - minValue) * scaleY;
             double x2 = leftMargin + i * scaleX;
             double y2 = panelHeight - bottomMargin - (values[i] - minValue) * scaleY;
             dc.DrawLine(wxPoint(x1, y1), wxPoint(x2, y2));
         }
         
         // Draw individual data points as small circles.
         for (size_t i = 0; i < values.size(); ++i) {
             double x = leftMargin + i * scaleX;
             double y = panelHeight - bottomMargin - (values[i] - minValue) * scaleY;
             dc.DrawCircle(wxPoint(x, y), 3);
         }
     
         // Annotate the Y-axis with value labels.
         dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
         for (int i = 0; i <= horizontalDivisions; ++i) {
             int y = topMargin + i * gridStepY;
             double valueLabel = maxValue - i * (yRange / horizontalDivisions);
             wxString label;
             label.Printf("%.2f", valueLabel);
             dc.DrawText(label, 5, y - 7);
         }
     
         // Annotate the X-axis with date labels.
         for (size_t i = 0; i < values.size(); i += labelInterval) {
             double x = leftMargin + i * scaleX;
             wxString dateLabel = wxString::FromUTF8(dates[i].c_str());
             dc.DrawRotatedText(dateLabel, x, panelHeight - bottomMargin + 125, 90);
         }
     
         // Identify indices for the minimum and maximum values.
         auto minIt = min_element(values.begin(), values.end());
         auto maxIt = max_element(values.begin(), values.end());
         int minIndex = distance(values.begin(), minIt);
         int maxIndex = distance(values.begin(), maxIt);
     
         // Highlight and label the minimum data point.
         {
             double x = leftMargin + minIndex * scaleX;
             double y = panelHeight - bottomMargin - (values[minIndex] - minValue) * scaleY;
             wxPen highlightPen(*wxBLUE_PEN);
             highlightPen.SetWidth(2);
             dc.SetPen(highlightPen);
             dc.DrawCircle(wxPoint(x, y), 5);
             wxString minLabel;
             minLabel.Printf("Min: %.2f (%s)", values[minIndex], wxString::FromUTF8(dates[minIndex].c_str()));
             dc.DrawText(minLabel, x + 5, y - 10);
         }
     
         // Highlight and label the maximum data point.
         {
             double x = leftMargin + maxIndex * scaleX;
             double y = panelHeight - bottomMargin - (values[maxIndex] - minValue) * scaleY;
             wxPen highlightPen(*wxRED_PEN);
             highlightPen.SetWidth(2);
             dc.SetPen(highlightPen);
             dc.DrawCircle(wxPoint(x, y), 5);
             wxString maxLabel;
             maxLabel.Printf("Max: %.2f (%s)", values[maxIndex], wxString::FromUTF8(dates[maxIndex].c_str()));
             dc.DrawText(maxLabel, x + 5, y - 10);
         }
     
         // Calculate and display additional statistics.
         double sum = accumulate(values.begin(), values.end(), 0.0);
         double avg = (values.empty() ? 0 : sum / values.size());
         wxString currentValueStr;
         wxString printMin;
         wxString printMax;
         wxString avgStr;
         wxString printTrend;
     
         currentValueStr.Printf("Current Value: %.2f", values.back());
         printMin.Printf("Min: %.2f (%s)", values[minIndex], wxString::FromUTF8(dates[minIndex].c_str()));
         printMax.Printf("Max: %.2f (%s)", values[maxIndex], wxString::FromUTF8(dates[maxIndex].c_str()));
         avgStr.Printf("Average Value: %.2f", avg);
         printTrend.Printf("Trend: %s", CalculateTrend(values));
         wxFont currentValueFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
         dc.SetFont(currentValueFont);
         dc.SetPen(*wxBLACK_PEN);
         dc.DrawText(currentValueStr, leftMargin, panelHeight - 40);
         dc.DrawText(printMin, leftMargin + 175, panelHeight - 40);
         dc.DrawText(printMax, leftMargin + 425, panelHeight - 40);
         dc.DrawText(avgStr, leftMargin, panelHeight - 20);
         dc.DrawText(printTrend, leftMargin + 175, panelHeight - 20);
     }
     
     /**
      * @brief Calculates the trend of the sensor data.
      *
      * Uses a simple linear regression on the data points to determine if the trend is rising, falling, or stable.
      * 10 degree used in order to determine which one it is
      *
      * @param values Vector containing sensor values.
      * @return A string indicating the trend: "Rising", "Falling", "Stable", or an error message if data is insufficient.
      */
     string CalculateTrend(const vector<double>& values) {
         if (values.size() < 2) {
             return "Not enough data";
         }
         
         size_t n = values.size();
         double sumX = 0;
         double sumY = 0;
         double sumXY = 0;
         double sumX2 = 0;
         
         // Use the index as the x-coordinate.
         for (size_t i = 0; i < n; ++i) {
             double x = static_cast<double>(i);
             double y = values[i];
             sumX += x;
             sumY += y;
             sumXY += x * y;
             sumX2 += x * x;
         }
         
         // Compute the slope using the linear regression formula.
         double denominator = n * sumX2 - sumX * sumX;
         if (denominator == 0) {
             return "Undefined trend";
         }
         double slope = (n * sumXY - sumX * sumY) / denominator;
         double slopdeg = 0.1763;
         if (slope > slopdeg) {
             return "Rising";
         } else if (slope < -slopdeg) {
             return "Falling";
         } else {
             return "Stable";
         }
     }
 };
 
 /**
  * @brief Callback for libcurl to write received data.
  *
  * This function is called by libcurl as soon as data is received and appends it to a string.
  *
  * @param contents Pointer to the data received.
  * @param size Size of a single element.
  * @param nmemb Number of elements.
  * @param output Pointer to the string to which data is appended.
  * @return The total number of bytes processed.
  */
 size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
     size_t totalSize = size * nmemb;
     output->append((char*)contents, totalSize);
     return totalSize;
 }
 
 /**
  * @brief Fetches JSON data from a URL and saves it to a file.
  *
  * Uses libcurl to perform an HTTP GET request, parses the response as JSON,
  * and writes the formatted JSON to the specified file.
  *
  * @param url The URL to fetch data from.
  * @param filename The file to which the JSON data will be written.
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
 
         if (result == CURLE_OK) {
             try {
                 nlohmann::json jsonData = nlohmann::json::parse(responseString);
                 ofstream file(filename);
                 if (file.is_open()) {
                     file << jsonData.dump(4);
                     file.close();
                 }
             } catch (nlohmann::json::parse_error& e) {
                 cerr << "JSON Parsing Error: " << e.what() << endl;
             }
         }
 
         curl_easy_cleanup(curl);
     }
     curl_global_cleanup();
 }
 
 /**
  * @brief Initializes sensor station data.
  *
  * Checks if a local JSON file exists and is non-empty.
  * If not, or if the user requests an update, it downloads the station database.
  *
  * @param parent Pointer to the parent wxWindow for dialog messages.
  */
 void init(wxWindow* parent) {
     int response;
     ifstream file("findAllmine.json");
     if (!file.is_open() || file.peek() == ifstream::traits_type::eof()) {
         file.close();
         fetchAndSaveData("https://api.gios.gov.pl/pjp-api/rest/station/findAll", "findAllmine.json");
     } else {
         response = wxMessageBox("Do you wish to download the database?", "Update Database", wxYES_NO | wxICON_QUESTION, parent);
         if (response == wxYES) {
             file.close();
             fetchAndSaveData("https://api.gios.gov.pl/pjp-api/rest/station/findAll", "findAllmine.json");
         } else {
             cout << "User chose not to update the database.\n";
         }
     }
 
     // Process the station data into a simplified database.
     ifstream outFile("database.json");
     if (response == wxYES || !outFile.is_open() || outFile.peek() == ifstream::traits_type::eof()) {
         outFile.close();
         file.open("findAllmine.json");
         file.clear();
 
         nlohmann::json jsonData;
         file >> jsonData;
         file.close();
 
         nlohmann::json jsonDatabase;
         for (const auto& station : jsonData) {
             if (station.contains("city")) {
                 string cityName = station["city"]["name"].get<string>();
                 int id = station["id"].get<int>();
                 string provinceName = station["city"]["commune"]["provinceName"].get<string>();
                 double georgeLat = stod(station["gegrLat"].get<string>());
                 double geogreLon = stod(station["gegrLon"].get<string>());
 
                 nlohmann::json newEntry;
                 newEntry["id"] = id;
                 newEntry["provinceName"] = provinceName;
                 newEntry["cityName"] = cityName;
                 newEntry["gegrLat"] = georgeLat;
                 newEntry["geogrLon"] = geogreLon;
                 jsonDatabase.push_back(newEntry);
             }
         }
 
         ofstream outFile("database.json");
         if (!outFile.is_open()) {
             cerr << "Could not open database.json for writing!" << endl;
         }
         outFile << jsonDatabase.dump(4);
     }
     outFile.close();
 }
 
 /**
  * @brief Fetches sensor data for a specific sensor of a station.
  *
  * Retrieves sensor data via HTTP request and updates the local JSON file for the station.
  *
  * @param stationID Identifier of the station.
  * @param sensorID Identifier of the sensor.
  */
 void fetchAndSaveSensorData(int stationID, int sensorID) {
     CURL* curl;
     CURLcode result;
     string responseString;
 
     curl_global_init(CURL_GLOBAL_DEFAULT);
     curl = curl_easy_init();
 
     if (curl) {
         string url = "https://api.gios.gov.pl/pjp-api/rest/data/getData/" + to_string(sensorID);
         curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
         curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
 
         result = curl_easy_perform(curl);
 
         if (result == CURLE_OK) {
             try {
                 nlohmann::json newSensorData = nlohmann::json::parse(responseString);
                 string filename = to_string(stationID) + ".json";
                 ifstream file(filename);
                 nlohmann::json stationData;
 
                 if (file.is_open()) {
                     file >> stationData;
                     file.close();
                 }
 
                 bool updated = false;
                 for (auto& sensor : stationData) {
                     if (sensor["id"] == sensorID) {
                         sensor["values"] = newSensorData["values"];
                         updated = true;
                         break;
                     }
                 }
 
                 if (!updated) {
                     nlohmann::json newEntry;
                     newEntry["id"] = sensorID;
                     newEntry["values"] = newSensorData["values"];
                     stationData.push_back(newEntry);
                 }
 
                 ofstream outFile(filename);
                 if (outFile.is_open()) {
                     outFile << stationData.dump(4);
                     outFile.close();
                 }
             } catch (nlohmann::json::parse_error& e) {
                 cerr << "JSON Parsing Error: " << e.what() << endl;
             }
         }
 
         curl_easy_cleanup(curl);
     }
     curl_global_cleanup();
 }
 
 /**
  * @brief Updates the sensor data for a station.
  *
  * Constructs the proper URL using the station ID and calls fetchAndSaveData().
  *
  * @param stationID Identifier of the station.
  */
 void updateData(int stationID) {
     fetchAndSaveData("https://api.gios.gov.pl/pjp-api/rest/station/sensors/" + to_string(stationID), (to_string(stationID) + ".json"));
 }
 
 /**
  * @class MyFrame
  * @brief Main window frame for the Professional App.
  *
  * Contains UI elements such as search box, buttons, and result list.
  * Handles searching and displaying sensor station data.
  */
 class MyFrame : public wxFrame {
 public:
     /**
      * @brief Constructs the main application frame.
      */
     MyFrame() : wxFrame(nullptr, wxID_ANY, "Professional App", wxDefaultPosition, wxSize(400, 400)) {
         wxPanel* panel = new wxPanel(this);
         wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
 
         searchBox = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(200, 30), wxTE_PROCESS_ENTER);
         searchBox->Bind(wxEVT_TEXT_ENTER, &MyFrame::OnSearch, this);
         sizer->Add(searchBox, 0, wxALL | wxCENTER, 10);
 
         wxButton* searchBtn = new wxButton(panel, wxID_ANY, "Search City");
         sizer->Add(searchBtn, 0, wxALL | wxCENTER, 10);
         searchBtn->Bind(wxEVT_BUTTON, &MyFrame::OnSearch, this);
 
         wxButton* updateBtn = new wxButton(panel, wxID_ANY, "Update Station Data");
         sizer->Add(updateBtn, 0, wxALL | wxCENTER, 10);
         updateBtn->Bind(wxEVT_BUTTON, &MyFrame::OnUpdate, this);
 
         resultList = new wxListBox(panel, wxID_ANY, wxDefaultPosition, wxSize(300, 150));
         sizer->Add(resultList, 1, wxEXPAND | wxALL, 10);
         resultList->Bind(wxEVT_LISTBOX_DCLICK, &MyFrame::OnCitySelected, this);
 
         panel->SetSizer(sizer);
     }
 
 private:
     wxTextCtrl* searchBox; ///< Text control for city search input.
     wxListBox* resultList; ///< List box displaying search results.
     vector<nlohmann::json> cityResults; ///< Vector of cities from the database.
 
     /**
      * @brief Handles the search event.
      *
      * Reads the local database, searches for a city by name, and displays matching results.
      *
      * @param event The search command event.
      */
     void OnSearch(wxCommandEvent&) {
         ifstream dataFile("database.json");
         if (!dataFile.is_open()) {
             wxMessageBox("Database file not found!", "Error", wxICON_ERROR);
             return;
         }
     
         nlohmann::json dataFromDatabase;
         dataFile >> dataFromDatabase;
         dataFile.close();
     
         wxString input = searchBox->GetValue();
         cityResults.clear();
         resultList->Clear();
     
         for (const auto& city : dataFromDatabase) {
             wxString cityName = wxString::FromUTF8(city["cityName"].get<string>());
             if (cityName.IsSameAs(input, false)) {
                 cityResults.push_back(city);
                 int CityId = city["id"];
                 resultList->Append(cityName + " (" + to_string(CityId) + ")");
             }
         }
         // If no city is found, offer a coordinate search.
         if (cityResults.empty()) {
             wxMessageBox("City not found in database.", "Search Result", wxICON_WARNING);
             int response = wxMessageBox("Do you wish to search city through coordinates?", "Update Database", wxYES_NO | wxICON_QUESTION, this);
 
             if (response == wxYES) {
                 wxString latStr = wxGetTextFromUser("Enter Latitude:", "Input Coordinates");
                 wxString lonStr = wxGetTextFromUser("Enter Longitude:", "Input Coordinates");
                 
                 double userLat, userLon;
                 if (!latStr.ToDouble(&userLat) || !lonStr.ToDouble(&userLon)) {
                     wxMessageBox("Invalid coordinates input!", "Error", wxICON_ERROR);
                     return;
                 }
                 
                 double minDistance = numeric_limits<double>::max();
                 nlohmann::json closestStation;
                 for (const auto& station : dataFromDatabase) {
                     double stationLat = station["gegrLat"].get<double>();
                     double stationLon = station["geogrLon"].get<double>();
                     double distance = Haversine(userLat, userLon, stationLat, stationLon);
                     if (distance < minDistance) {
                         minDistance = distance;
                         closestStation = station;
                     }
                 }
                 
                 cityResults.push_back(closestStation);
                 int stationID = closestStation["id"].get<int>();
                 wxString stationName = wxString::FromUTF8(closestStation["cityName"].get<string>());
                 wxString listItem;
                 listItem.Printf("%s (ID: %d) - closest %f", stationName, stationID, minDistance);
                 resultList->Append(listItem);
             } else {
                 cout << "Naaaah we're going through name\n";
             }
         }
     }
 
     /**
      * @brief Placeholder for Haversine calculation.
      *
      * Computes a simple difference between two coordinates.
      * @note This is a simplified version and does not calculate the actual great-circle distance.
      *
      * @param lat1 Latitude of the first coordinate.
      * @param lon1 Longitude of the first coordinate.
      * @param lat2 Latitude of the second coordinate.
      * @param lon2 Longitude of the second coordinate.
      * @return A double representing the coordinate difference.
      */
     double Haversine(double lat1, double lon1, double lat2, double lon2) {
         double diff;
         diff = lat1 - lat2;
         diff += lon1 - lon2;
         return diff;
     }
 
     /**
      * @brief Handles selection of a city from the result list.
      *
      * When a city is double-clicked, this function displays its details.
      *
      * @param event The list box selection event.
      */
     void OnCitySelected(wxCommandEvent&) {
         int selection = resultList->GetSelection();
         if (selection != wxNOT_FOUND) {
             ShowCityDetails(cityResults[selection]);
         }
     }
 
     /**
      * @brief Displays sensor data details for a specific sensor.
      *
      * Opens a dialog that contains a graph and date range controls for filtering the sensor data.
      *
      * @param stationID The station identifier.
      * @param sensorID The sensor identifier.
      */
     void ShowSensorData(int stationID, int sensorID) {
         string filename = to_string(stationID) + ".json";
         ifstream stationFile(filename);
         nlohmann::json stationData;
         
         if (!stationFile.is_open()) {
             wxMessageBox("Station data not found! Please fetch data first.", "Error", wxICON_ERROR);
             return;
         }
         stationFile >> stationData;
         stationFile.close();
     
         // Gather sensor data that contains valid "values".
         vector<nlohmann::json> fullSensorData;
         if (auto it = find_if(stationData.begin(), stationData.end(), [](const auto& elem) {
             return elem.contains("values");
         }); it != stationData.end()) {
             int response = wxMessageBox("Do you wish to Download Sensor data?", "Update Database", wxYES_NO | wxICON_QUESTION, this);
             if (response == wxYES) {
                 fetchAndSaveSensorData(stationID, sensorID);
                 wxMessageBox("Data downloaded. Please reopen to view graph.", "Info", wxICON_INFORMATION);
             } else {
                 cout << "User chose not to update the station database.\n";
             
                 for (const auto& sensor : stationData) {
                     if (sensor["id"] == sensorID && sensor.contains("values")) {
                         fullSensorData.push_back(sensor);
                     }
                 }
     
                 // Create a dialog for displaying sensor data with date filters.
                 wxDialog* detailsDialog = new wxDialog(this, wxID_ANY, "Sensor Data Graph", wxDefaultPosition, wxSize(900, 700));
                 wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
     
                 // --- Date range selection controls ---
                 wxPanel* controlPanel = new wxPanel(detailsDialog);
                 wxBoxSizer* controlSizer = new wxBoxSizer(wxHORIZONTAL);
     
                 wxStaticText* startLabel = new wxStaticText(controlPanel, wxID_ANY, "Start Date:");
                 wxDatePickerCtrl* startDatePicker = new wxDatePickerCtrl(controlPanel, wxID_ANY);
                 wxStaticText* endLabel = new wxStaticText(controlPanel, wxID_ANY, "End Date:");
                 wxDatePickerCtrl* endDatePicker = new wxDatePickerCtrl(controlPanel, wxID_ANY);
                 wxButton* applyButton = new wxButton(controlPanel, wxID_ANY, "Apply Date Range");
     
                 controlSizer->Add(startLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
                 controlSizer->Add(startDatePicker, 0, wxALL, 5);
                 controlSizer->Add(endLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
                 controlSizer->Add(endDatePicker, 0, wxALL, 5);
                 controlSizer->Add(applyButton, 0, wxALL, 5);
                 controlPanel->SetSizer(controlSizer);
                 mainSizer->Add(controlPanel, 0, wxEXPAND | wxALL, 10);
     
                 // --- Graph panel ---
                 GraphPanel* graphPanel = new GraphPanel(detailsDialog, fullSensorData);
                 graphPanel->SetMinSize(wxSize(1000, 550));
                 graphPanel->SetSize(wxSize(1000, 550));
                 mainSizer->Add(graphPanel, 1, wxEXPAND | wxALL, 10);
     
                 // --- Close button ---
                 wxButton* closeButton = new wxButton(detailsDialog, wxID_OK, "Close");
                 mainSizer->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 10);
     
                 detailsDialog->SetSizerAndFit(mainSizer);
                 detailsDialog->Layout();
     
                 // --- Bind the apply button to filter data by date range ---
                 applyButton->Bind(wxEVT_BUTTON, [=, &fullSensorData, &graphPanel, &mainSizer, &detailsDialog](wxCommandEvent&) mutable {
                     wxDateTime start = startDatePicker->GetValue();
                     wxDateTime end = endDatePicker->GetValue();
                     // Filter sensor data based on selected date range.
                     std::vector<nlohmann::json> filteredData = FilterSensorDataByDateRange(fullSensorData, start, end);
     
                     // Replace the old graph panel with a new one using filtered data.
                     mainSizer->Detach(graphPanel);
                     graphPanel->Destroy();
                     graphPanel = new GraphPanel(detailsDialog, filteredData);
                     graphPanel->SetMinSize(wxSize(1000, 550));
                     graphPanel->SetSize(wxSize(1000, 550));
                     mainSizer->Insert(1, graphPanel, 1, wxEXPAND | wxALL, 10);
                     detailsDialog->Layout();
                 });
     
                 detailsDialog->ShowModal();
                 detailsDialog->Destroy();
             }
         } else {
             fetchAndSaveSensorData(stationID, sensorID);
             wxMessageBox("Data downloaded. Please reopen to view graph.", "Info", wxICON_INFORMATION);
         }
     }
     
     /**
      * @brief Filters sensor data entries based on a date range.
      *
      * Iterates through the sensor data and returns a new vector that only includes entries with dates within the specified range.
      *
      * @param sensorData Vector of sensor data JSON objects.
      * @param startDate The start date for filtering.
      * @param endDate The end date for filtering.
      * @return A vector of JSON objects with sensor data entries within the date range.
      */
     std::vector<nlohmann::json> FilterSensorDataByDateRange(
         const std::vector<nlohmann::json>& sensorData,
         const wxDateTime& startDate,
         const wxDateTime& endDate)
     {
         std::vector<nlohmann::json> filteredData;
         for (const auto& sensor : sensorData) {
             nlohmann::json sensorCopy;
             sensorCopy["id"] = sensor["id"];
             sensorCopy["values"] = nlohmann::json::array();
             for (const auto& dataEntry : sensor["values"]) {
                 // Skip if "value" is missing or null.
                 if (!dataEntry.contains("value") || dataEntry["value"].is_null())
                     continue;
                 
                 std::string dateStd = dataEntry.value("date", "1970-01-01");
                 wxString dateStr = wxString::FromUTF8(dateStd.c_str());
                 wxDateTime dt;
                 dt.ParseFormat(dateStr, "%Y-%m-%d");
                 if (dt.IsValid() && dt >= startDate && dt <= endDate) {
                     sensorCopy["values"].push_back(dataEntry);
                 }
             }
             if (!sensorCopy["values"].empty())
                 filteredData.push_back(sensorCopy);
         }
         return filteredData;
     }
     
     /**
      * @brief Displays details for a selected city.
      *
      * Loads the station data from file and displays available sensor parameters in a dialog.
      *
      * @param city JSON object representing the city.
      */
     void ShowCityDetails(const nlohmann::json& city) {
         int stationID = city["id"].get<int>();
         string filename = to_string(stationID) + ".json";
         ifstream stationFile(filename);
     
         if (!stationFile.is_open() || stationFile.peek() == ifstream::traits_type::eof()) {
             updateData(stationID);
             wxMessageBox("Fetching data... Try again in a few seconds.", "Info", wxICON_INFORMATION);
             return;
         }
     
         nlohmann::json stationData;
         stationFile >> stationData;
         stationFile.close();
     
         wxDialog* detailsDialog = new wxDialog(this, wxID_ANY, "Sensor Parameters", wxDefaultPosition, wxSize(400, 350));
         wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
         wxListBox* sensorList = new wxListBox(detailsDialog, wxID_ANY, wxDefaultPosition, wxSize(350, 200));
     
         vector<nlohmann::json> sensorDetails;
         for (auto& sensor : stationData) {
             wxString paramName = wxString::FromUTF8(sensor["param"]["paramName"].get<string>());
             sensorList->Append(paramName);
             sensorDetails.push_back(sensor);
         }
     
         vbox->Add(sensorList, 1, wxEXPAND | wxALL, 10);
         wxButton* closeButton = new wxButton(detailsDialog, wxID_OK, "Close");
         vbox->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 10);
     
         // When a sensor is double-clicked, show its detailed data.
         sensorList->Bind(wxEVT_LISTBOX_DCLICK, [this, sensorList, &sensorDetails, stationID](wxCommandEvent&) {
             int selection = sensorList->GetSelection();
             if (selection != wxNOT_FOUND) {
                 int sensorID = sensorDetails[selection]["id"].get<int>();
                 this->ShowSensorData(stationID, sensorID);
             }
         });
     
         detailsDialog->SetSizer(vbox);
         detailsDialog->ShowModal();
         detailsDialog->Destroy();
     }
     
     /**
      * @brief Handles update requests for station data.
      *
      * Prompts the user to enter a station ID and then calls updateData() to refresh its sensor data.
      *
      * @param event The update command event.
      */
     void OnUpdate(wxCommandEvent&) {
         wxString stationID = wxGetTextFromUser("Enter Station ID to update:", "Update Data");
         if (!stationID.IsEmpty()) {
             updateData(stoi(stationID.ToStdString()));
         }
     }
 };
 
 /**
  * @class MyApp
  * @brief wxApp-derived class that initializes the application.
  *
  * Sets locale, creates the main frame, and initializes station data.
  */
 class MyApp : public wxApp {
 public:
     /**
      * @brief Application initialization.
      *
      * Sets the locale to UTF-8, creates the main window, initializes the sensor database,
      * and displays the main window.
      *
      * @return true if initialization is successful.
      */
     virtual bool OnInit() {
         wxSetlocale(LC_ALL, "en-US.UTF-8");
         MyFrame* frame = new MyFrame();
         init(frame);
         frame->Show(true);
         return true;
     }
 };
 
 wxIMPLEMENT_APP(MyApp);
 