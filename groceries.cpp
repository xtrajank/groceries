#include "split.h"
#include <sstream>
#include <string>
#include <list>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;

// stores all customer information
struct Customer {
    int customer_id;
    string name;
    string street;
    string city;
    string state;
    string zip;
    string phone;
    string email;

    string print_detail() const {
        stringstream ss;

        ss << "Customer ID #" << customer_id << ":\n"
        << name << " ph. " << phone << ", email: " << email << "\n"
        << street << "\n" << city << ", " << state << " " << zip << endl;

        return ss.str();
    }
};

// stores all item info
struct Item {
    int item_id;
    string description;
    double price;
};

// stores everything necessary for each item read from orders.txt. returns total for that item.
struct LineItem {
    Item item;
    int quantity;

    double sub_total() const {
        return item.price * quantity;
    }
};

// stores payment amount, set up as abstract so it can be changed by payment type.
struct Payment {
    double amount;

    virtual string print_detail() const = 0;

    virtual ~Payment () {}
};

// strores credit payment info. inherits Payment abstract class. Outputs credit info into nice line.
struct Credit : public Payment {
    string card_number;
    string expiration;

    string print_detail() const override {
        stringstream ss;

        ss << "Amount: $" << fixed << setprecision(2) << amount << ", Paid by Credit card " << card_number << ", exp. " << expiration << endl;

        return ss.str();
    }
};

// strores paypal payment info. inherits Payment abstract class. Outputs paypal info into nice line.
struct PayPal : public Payment {
    string paypal_id;

    string print_detail() const override {
        stringstream ss;

        ss << "Amount: $" << fixed << setprecision(2) << amount << ", Paid by Paypal ID: " << paypal_id << endl;

        return ss.str();
    }
};

// strores wiretransfer payment info. inherits Payment abstract class. Outputs wiretransfer info into nice line.
struct WireTransfer : public Payment {
    string bank_id;
    string account_id;

    string print_detail() const override {
        stringstream ss;

        ss << "Amount: $" << fixed << setprecision(2) << amount << ", Paid by Wire transfer from Bank ID " << bank_id << ", Account # " << account_id << endl;

        return ss.str();
    }
};

// stores all order info
struct Order {
    int order_id;
    string order_date;
    double sum = 0.0;
    Customer* customer;
    vector<LineItem> line_items;
    Payment* payment; // pointer to Payment so that the class is mutable

    // adds the total of all the items in one order together
    double total () {
        sum = 0.0;
        for (const auto& line_item : line_items) {
            sum += line_item.sub_total();
        }
    
        payment->amount = sum;

        return sum; // return the grand total
    }

    // prints order nice
    string print_order () const{
        stringstream ss;
        ss << setfill('-') << setw(50);
        ss << "\nOrder #" << order_id << ", Date: " << order_date << endl << endl;
        ss << payment->print_detail() << endl;
        if (payment && payment->amount != sum) {
            cerr << "Debug: Payment amount (" << payment->amount << ") does not match order sum (" << sum << ") for order ID " << order_id << endl;
        }

        ss << customer->print_detail() << endl;

        ss << "Order Detail:" << endl;
        for (const auto& line_item : line_items) {
            ss << "\tItem " << line_item.item.item_id << ": \""
            << line_item.item.description << "\", " << line_item.quantity 
            << " @ " << fixed << setprecision(2) << line_item.item.price << endl;
        }
        return ss.str();
    }
};

// static global vectors to store all customers and items, accessible within this file only
static vector<Customer> customers;
static vector<Item> items;
static list<Order> orders;

// opens .txt file with all customer data. reads line by line and seperates customer data into respective class attribute.
void read_customers(const string& file) {
    string line;
    int customer_count = 0;
    ifstream customer_file(file);
    if (customer_file.is_open()) {
        while (getline(customer_file, line)) {
        auto fields = split(line, ',');
        customer_count++;
            if (fields.size() == 8) {
            Customer cust;
            cust.customer_id = stoi(fields[0]);
            cust.name = fields[1];
            cust.street = fields[2];
            cust.city = fields[3];
            cust.state = fields[4];
            cust.zip = fields[5];
            cust.phone = fields[6];
            cust.email = fields[7];
            customers.push_back(cust);
            } else {
                cerr << "Error: Not enough fields in line. " << line << endl;
            }
        }
        customer_file.close();
    } else {
        cerr << "Error: File not open. " << file << endl;
    }
}

// opens .txt file with all item data. reads line by line and seperates item data into respective class attribute.
void read_items(const string& file) {
    string line;
    int item_count = 0;
    ifstream item_file(file);
    if(item_file.is_open()){
        while (getline(item_file, line)) {
        item_count++;
        auto fields = split(line, ',');
            if (fields.size() == 3) {
                Item itm;
                itm.item_id = stoi(fields[0]);
                itm.description = fields[1];
                itm.price = stod(fields[2]);
                items.push_back(itm);
            } else {
            cerr << "Error: Not enough fields in this line. " << line << endl;
            }
        }
        item_file.close();
    } else {
        cerr << "Error: file not open. " << file << endl;
    }
}

// reads each order in orders.txt file
void read_orders(const string& file) {
    string line;
    ifstream order_file(file);
    if (order_file.is_open()) {
        while (getline(order_file, line)) {
            auto fields = split(line, ',');
            Order order;

            // Check if it's an order line (with more than three fields)
            if (fields.size() > 3) {
                // Parse customer ID, order ID, and order date
                int customer_id = stoi(fields[0]);
                order.order_id = stoi(fields[1]);
                order.order_date = fields[2];

                // Find and set the customer
                Customer* cust = nullptr;
                for (auto& c : customers) {
                    if (c.customer_id == customer_id) {
                        cust = &c;
                        break;
                    }
                }
                if (cust) {
                    order.customer = cust;
                } else {
                    cerr << "Error: Customer ID " << customer_id << " not found." << endl;
                    continue;
                }

                // Process line items starting from the fourth field
                for (int i = 3; i < fields.size(); i++) {
                    auto item_data = split(fields[i], '-');
                    int item_id = stoi(item_data[0]);
                    int quantity = stoi(item_data[1]);

                    // Find and set the item
                    Item* item = nullptr;
                    for (auto& itm : items) {
                        if (itm.item_id == item_id) {
                            item = &itm;
                            break;
                        }
                    }
                    if (item) {
                        LineItem line_item = {*item, quantity};
                        order.line_items.push_back(line_item);
                    } else {
                        cerr << "Error: Item ID " << item_id << " not found." << endl;
                    }
                }

                // Read the following line to set payment details
                if (getline(order_file, line)) {
                    fields = split(line, ',');

                    // Identify and set the payment type
                    if (fields.size() <= 3) {
                        if (fields[0] == "1") {
                            Credit* credit = new Credit;
                            credit->card_number = fields[1];
                            credit->expiration = fields[2];
                            order.payment = credit;
                        } else if (fields[0] == "2") {
                            PayPal* paypal = new PayPal;
                            paypal->paypal_id = fields[1];
                            order.payment = paypal;
                        } else if (fields[0] == "3") {
                            WireTransfer* wire = new WireTransfer;
                            wire->bank_id = fields[1];
                            wire->account_id = fields[2];
                            order.payment = wire;
                        }
                    }
                }

                // Ensure that the payment is set up
                if (order.payment == nullptr) {
                    cerr << "Error: Payment not initialized for order ID " << order.order_id << endl;
                    continue;
                }
                // Calculate total and add order to list
                order.total();
                orders.push_back(order);

            } else {
                cerr << "Error: Unexpected format in line: " << line << endl;
            }
        }
        order_file.close();
    } else {
        cerr << "Error: Could not open file " << file << endl;
    }
}

void one_customer_order() {
    // customer and item count display
    cout << "Customers: " << customers.size() << " Items: " << items.size() << endl;

    // ask for and store desired customer id
    int customerInput;
    cout << "Enter customer id: ";
    cin >> customerInput;

    // search customer list for customer id
    bool cust_found = false;
    for (const auto& cust : customers) {
        if (cust.customer_id == customerInput) {
            cust_found = true;
            break;
        }
    }
    if (!cust_found) {
        cout << "Customer with id " << customerInput << " not found." << endl;
        return;
    }

    // search item list for item id and keeps track
    int purchased = 0;
    double total = 0.0;
    while (true) {
        int itemInput;
        cout << "Enter item id (0 to exit): ";
        cin >> itemInput;
        if (itemInput == 0) {
            break;
        }
        bool item_found = false;
        for (const auto& itm : items) {
            if (itm.item_id == itemInput) {
                total += itm.price;
                purchased++;
                item_found = true;
            }
        }
        if (!item_found) {
            cout << "Item not found: " << itemInput << endl;
        }
    }
    cout << "Number of items purchased: " << purchased << " Total: $" << fixed <<
    setprecision(2) << total << endl;
}

int main() {
    read_customers("customers.txt");
    read_items("items.txt");
    read_orders("orders.txt");

    ofstream ofs("order_report.txt");
    for (const auto& order: orders) {
        ofs << order.print_order() << endl;
    }
}