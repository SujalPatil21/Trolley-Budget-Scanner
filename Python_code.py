
import psycopg2
import serial
import time

# Database connection details
try:
    conn = psycopg2.connect(
        dbname="trolley_scanner",
        user="postgres",
        password="postgre",
        host="localhost",
        port="5432"
    )
    print("Database connected successfully!")
except Exception as e:
    print(f"Error connecting to the database: {e}")

# Initialize serial connection (replace 'COM4' with your actual port)
arduino = serial.Serial('COM2', 9600)
time.sleep(2)  # Wait for the Arduino to reset

# List to store scanned products (barcode, product name, price)
scanned_products = []

def get_product(barcode):
    try:
        with conn.cursor() as cur:
            cur.execute("SELECT product_name, price FROM products WHERE barcode_id = %s", (barcode,))
            product = cur.fetchone()
            if product:
                print(f"Product found: {product}")  # Debug print
            else:
                print("Product not found in database.")  # Debug print
            return product if product else None
    except Exception as e:
        print(f"Error fetching product: {e}")  # Debug print

def remove_product(barcode):
    global scanned_products
    for i, (scanned_barcode, product_name, price) in enumerate(scanned_products):
        if scanned_barcode == barcode:
            # Remove the product from the list
            del scanned_products[i]
            print(f"Removed product: {product_name}, {price}")  # Debug print
            return product_name, price
    return None

def main():
    total = 0
    product_count = 0

    print("Trolley Budget Scanner")
    print("=======================")

    while True:
        barcode = input("Scan a product : ").strip()
        print(f"Barcode scanned: {barcode}")  # Debug print

        if barcode.lower() == 'done':
            break

        if barcode == '*':
            # Handle product removal
            barcode_to_remove = input("Scan the product to remove: ").strip()
            removed_product = remove_product(barcode_to_remove)
            if removed_product:
                product_name, price = removed_product
                total -= price
                product_count -= 1
                print(f"Removed {product_name}. Updated total: {total:.2f}, Items: {product_count}")

                # Send the removal info to Arduino
                arduino.write(f"REMOVE:{product_name},{price}\n".encode())
                print(f"Sent to Arduino: REMOVE:{product_name},{price}")  # Debug print

                # Send updated total and item count to Arduino
                arduino.write(f"TOTAL_PRICE:{total:.2f}\n".encode())
                arduino.write(f"ITEM_COUNT:{product_count}\n".encode())
            else:
                print("Product not found in scanned list.")
            continue

        # Handle adding a product
        product = get_product(barcode)
        if product:
            product_name, price = product

            # Add product to scanned list
            scanned_products.append((barcode, product_name, price))

            # Send product name and price to Arduino
            arduino.write(f"{product_name},{price}\n".encode())
            print(f"Sent to Arduino: {product_name},{price}")  # Debug print

            total += price
            product_count += 1

            # Send updated total and item count to Arduino
            arduino.write(f"TOTAL_PRICE:{total:.2f}\n".encode())
            arduino.write(f"ITEM_COUNT:{product_count}\n".encode())

        else:
            print("Product not found in database.")

    # After scanning, send final count and total to Arduino
    arduino.write(f"ITEM_COUNT:{product_count}\n".encode())
    print(f"Sent to Arduino: ITEM_COUNT:{product_count}")  # Debug pr8901030702242
    int
    arduino.write(f"TOTAL_PRICE:{total:.2f}\n".encode())
    print(f"Sent to Arduino: TOTAL_PRICE:{total:.2f}")  # Debug print

if __name__ == "__main__":
    main()


