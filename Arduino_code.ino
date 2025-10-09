#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust the address if necessary

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'} // Use '#' for exiting
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

float budget = 0;
float totalPrice = 0;
int totalItems = 0;
bool removeMode = false;  // Flag to indicate remove mode

// Arrays to store scanned product info
String productNames[20];
float productPrices[20];
int productCount = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  lcd.setCursor(0, 0);
  lcd.print("Enter budget");
  // Read budget input
  budget = readBudget();
  lcd.clear();
  lcd.print("Scan Product");
}

void loop() {
  char key = keypad.getKey();

  if (key == '*') {
    // Toggle remove mode on pressing '*'
    removeMode = !removeMode;

    lcd.clear();
    if (removeMode) {
      lcd.print("Scan to remove");
    } else {
      lcd.print("Scan Product");
    }
  } else if (key == '#') {
    // If '#' is pressed, exit the current screen
    exitScreen();
  } else if (key == 'A') {
    // If 'A' is pressed, show the remaining budget
    displayRemainingBudget();
  }

  if (removeMode) {
    // If in remove mode, handle removal
    handleRemoveMode();
  } else {
    // Check for Serial input for adding products
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      processProductInfo(input, true); // Process as adding
    }

    // Treat the key as a product barcode for adding if not in remove mode
    if (key && key != '*' && key != '#' && key != 'A') {
      String productInfo = String(key);
      processProductInfo(productInfo, true); // Process as adding
    }
  }
}

void displayRemainingBudget() {
  lcd.clear();
  lcd.print("Remaining Budget:");
  lcd.setCursor(0, 1);
  lcd.print(budget - totalPrice);
  delay(3000);  // Display for 2 seconds
  
  // After displaying remaining budget, return to "Total Price and Total Items" screen
  lcd.clear();
  lcd.print("Total Price: ");
  lcd.print(totalPrice);
  lcd.setCursor(0, 1);
  lcd.print("Total Items: ");
  lcd.print(totalItems);
  delay(2500); // Display for 2 seconds
}

void handleRemoveMode() {
  String barcode = "";

  // Check if Serial input is available for removing
  if (Serial.available() > 0) {
    barcode = Serial.readStringUntil('\n');
  } else {
    // Otherwise, check if keypad input is available
    char key = keypad.getKey();
    if (key) {
      barcode = String(key);
    }
  }

  if (barcode.length() > 0) {
    processProductInfo(barcode, false);  // Process as removing
  }
}

void exitScreen() {
  // Display thank you message
  lcd.clear();
  lcd.print("Thank You!");  // Display the thank you message
  delay(2000);             // Display for 2 seconds
  lcd.clear();

  // Reset variables to prepare for new budget input
  totalPrice = 0;
  totalItems = 0;
  productCount = 0; // Reset product count

  // Prompt for new budget
  lcd.print("Enter budget");
  budget = readBudget(); // Call the budget input function to allow new budget entry
  lcd.clear();
  lcd.print("Scan Product");
}

void checkBudgetLimit() {
  if (totalPrice >= budget) {
    for (int i = 0; i < 50; i++) {
      lcd.clear();
      delay(500); // Clear for 500 ms
      lcd.setCursor(0, 0);
      lcd.print("Limit Reached");
      delay(500);
      lcd.setCursor(0, 1);
      lcd.print("Thank You!       ");
      lcd.print(totalItems);
      delay(500); // Show for 500 ms

      // Check for '#' key during the blinking
      if (keypad.getKey() == '#') {
        exitScreen();  // Call the exit screen function
        return;  // Exit the function after resetting
      }
    }
    while (true); // Stop the program here after blinking
  }
}

float readBudget() {
  String input = "";
  char key;
  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key == 'B') { // Confirm the budget with 'B'
        break;
      } else if (key == 'C') { // Clear the entire input with 'C'
        input = ""; // Reset the input
        lcd.clear();
        lcd.print("Budget Cleared");
        delay(2000);
        lcd.clear();
        lcd.print("Enter budget");
      } else if (key == 'D') { // Delete the last digit with 'D'
        if (input.length() > 0) {
          input.remove(input.length() - 1);
          lcd.clear();
          lcd.print("Budget: " + input);
        }
      } else {
        input += key; // Add the digit to the input
        lcd.clear();
        lcd.print("Budget: " + input);
      }
    }
  }
  return input.toFloat();
}

void processProductInfo(String productInfo, bool isAdding) {
  // Debugging output for product info processing
  Serial.print("Processing product info: ");
  Serial.println(productInfo);

  if (productInfo == "Product not found") {
    lcd.clear();
    lcd.print(productInfo);
    delay(2000);
    lcd.clear();
    lcd.print("Scan Product");
    return;
  }

  int commaIndex = productInfo.indexOf(',');
  if (commaIndex != -1) {
    String productName = productInfo.substring(0, commaIndex);
    float price = productInfo.substring(commaIndex + 1).toFloat();

    if (isAdding) {
      // Store the product info for adding
      productNames[productCount] = productName; // Store product name
      productPrices[productCount] = price; // Store product price
      productCount++;

      displayProduct(productName, price);
      updateTotals(price, true);  // Use 'true' for adding
    } else {
      // Handle removal logic
      removeProduct(productName); // Call removeProduct directly with the name
    }
    
    checkBudgetLimit();
  }
}

void displayProduct(String productName, float price) {
  lcd.clear();
  lcd.print(productName);
  lcd.print(",");
  lcd.print(price);
  delay(2000);
}

void updateTotals(float price, bool isAdding) {
  if (isAdding) {
    totalPrice += price;
    totalItems++;
  } else {
    totalPrice -= price;
    totalItems--;
  }

  lcd.clear();
  lcd.print("Total Price: ");
  lcd.print(totalPrice);
  lcd.setCursor(0, 1);
  lcd.print("Total Items: ");
  lcd.print(totalItems);
  delay(2000);
}

void removeProduct(String productCode) {
  // Search for the product in the scanned list
  for (int i = 0; i < productCount; i++) {
    // Debugging output to check what we are comparing
    Serial.print("Comparing with: ");
    Serial.println(productNames[i]);
    Serial.print("Scanning for: ");
    Serial.println(productCode);

    if (productNames[i] == productCode) {
      float removedPrice = productPrices[i];

      // First, notify the user about the removal
      lcd.clear();
      lcd.print("Removed: ");
      lcd.print(productCode);
      delay(2000);

      // Now, decrement the total price and item count
      updateTotals(removedPrice, false);  // Use 'false' for removing

      // Shift the arrays to remove the product from the lists
      for (int j = i; j < productCount - 1; j++) {
        productNames[j] = productNames[j + 1];
        productPrices[j] = productPrices[j + 1];
      }
      productCount--;  // Decrease product count

      return;
    }
  }

  // If the product was not found, notify the user
  lcd.clear();
  lcd.print("Item not found");
  delay(2000);
}
