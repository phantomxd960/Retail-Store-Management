// gcc -o retail_system online_retail_store_management_system.c -lmysqlclient
//./retail_system
// msql -u root
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include <unistd.h>
#include <time.h>

#define HOST "localhost"
#define USER "root"
#define DB_PASSWORD "Ikpop1611B$"
#define DATABASE "retail_store"

MYSQL *conn;
int loggedInUser = -1;
char loggedInRole[10];

void connectDatabase()
{
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, HOST, USER, DB_PASSWORD, DATABASE, 0, NULL, 0))
    {
        fprintf(stderr, "Database connection failed: %s\n", mysql_error(conn));
        exit(1);
    }
}

void closeDatabase()
{
    mysql_close(conn);
}

void logAction(const char *action)
{
    if (loggedInUser == -1)
        return;
    char query[512];
    sprintf(query, "INSERT INTO audit_logs (user_id, action, timestamp) VALUES (%d, '%s', NOW())", loggedInUser, action);
    mysql_query(conn, query);
    FILE *logFile = fopen("system_logs.txt", "a");
    if (logFile)
    {
        time_t now = time(NULL);
        fprintf(logFile, "[%s] User %d: %s\n", ctime(&now), loggedInUser, action);
        fclose(logFile);
    }
}
void listOrders()
{
    if (mysql_query(conn, "SELECT id, user_id, product_id, quantity, status FROM orders"))
    {
        fprintf(stderr, "Failed to retrieve orders: %s\n", mysql_error(conn));
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    printf("\nOrders:\n");
    while ((row = mysql_fetch_row(res)))
    {
        printf("Order ID: %s, User ID: %s, Product ID: %s, Quantity: %s, Status: %s\n", row[0], row[1], row[2], row[3], row[4]);
    }
    mysql_free_result(res);
}

int loginUser(char *email, char *password)
{
    char query[256];
    MYSQL_RES *res;
    MYSQL_ROW row;
    sprintf(query, "SELECT id, role FROM users WHERE email='%s' AND password='%s'", email, password);

    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Login query failed: %s\n", mysql_error(conn));
        return -1;
    }
    res = mysql_store_result(conn);
    if ((row = mysql_fetch_row(res)))
    {
        loggedInUser = atoi(row[0]);
        strcpy(loggedInRole, row[1]);
        mysql_free_result(res);
        printf("Login successful! User ID: %d, Role: %s\n", loggedInUser, loggedInRole);
        logAction("User logged in");
        return loggedInUser;
    }
    mysql_free_result(res);

    printf("Invalid credentials!\n");

    // Ask the user if they want to register
    char choice[4];
    printf("Would you like to register as a new customer? (yes/no): ");
    scanf("%s", choice);

    if (strcmp(choice, "yes") == 0)
    {
        char name[50];
        char newEmail[50];
        char newPassword[50];

        // Get the details from the user
        printf("Enter your name: ");
        scanf("%s", name);
        printf("Enter your email: ");
        scanf("%s", newEmail);
        printf("Enter your password: ");
        scanf("%s", newPassword);

        // Insert the new customer into the database
        sprintf(query, "INSERT INTO users (name, email, password, role) VALUES ('%s', '%s', '%s', 'customer')", name, newEmail, newPassword);
        if (mysql_query(conn, query))
        {
            fprintf(stderr, "Registration failed: %s\n", mysql_error(conn));
            return -1;
        }

        printf("Registration successful! You can now log in.\n");
        return loginUser(newEmail, newPassword); // Call loginUser again to log the new user in
    }
    else if (strcmp(choice, "no") == 0)
    {
        printf("Thank you for visiting!\n");
        return -1; // Exit after thanking the user
    }
    else
    {
        printf("Invalid choice! Please enter 'yes' or 'no'.\n");
        return loginUser(email, password); // Retry login if the user enters an invalid option
    }
}

void logoutUser()
{
    if (loggedInUser != -1)
    {
        logAction("User logged out");
    }
    loggedInUser = -1;
    strcpy(loggedInRole, "");
    printf("User logged out successfully!\n");
}

void listUsers()
{
    if (mysql_query(conn, "SELECT id, name, email, role FROM users"))
    {
        fprintf(stderr, "Failed to retrieve users: %s\n", mysql_error(conn));
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    printf("\nUsers:\n");
    while ((row = mysql_fetch_row(res)))
    {
        printf("ID: %s, Name: %s, Email: %s, Role: %s\n", row[0], row[1], row[2], row[3]);
    }
    mysql_free_result(res);
}

void listProducts()
{
    if (mysql_query(conn, "SELECT id, name, price, stock FROM products"))
    {
        fprintf(stderr, "Failed to retrieve products: %s\n", mysql_error(conn));
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    printf("\nProducts:\n");
    while ((row = mysql_fetch_row(res)))
    {
        printf("ID: %s, Name: %s, Price: ₹%s, Stock: %s\n", row[0], row[1], row[2], row[3]);
    }
    mysql_free_result(res);
}

void addProduct()
{
    char name[100];
    double price;
    int stock;

    printf("Enter product name: ");
    scanf(" %[^\n]", name);

    // Check if product with the same name already exists
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT id, stock FROM products WHERE name = '%s'", name);

    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Database error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row;

    if ((row = mysql_fetch_row(result)))
    {
        // Product exists - update stock
        int productId = atoi(row[0]);
        int currentStock = atoi(row[1]);

        printf("Product already exists. Enter stock to add: ");
        scanf("%d", &stock);

        snprintf(query, sizeof(query),
                 "UPDATE products SET stock = stock + %d WHERE id = %d", stock, productId);

        if (mysql_query(conn, query) == 0)
        {
            printf("Stock updated successfully!\n");
            logAction("Updated existing product stock");
        }
        else
        {
            fprintf(stderr, "Failed to update stock: %s\n", mysql_error(conn));
        }
    }
    else
    {
        // New product - insert
        printf("Enter price: ");
        while (scanf("%lf", &price) != 1 || price < 0)
        {
            printf("Invalid input! Please enter a valid price: ");
            while (getchar() != '\n')
                ; // clear input buffer
        }

        printf("Enter stock: ");
        scanf("%d", &stock);

        snprintf(query, sizeof(query),
                 "INSERT INTO products (name, price, stock) VALUES ('%s', %.2f, %d)",
                 name, price, stock);

        if (mysql_query(conn, query) == 0)
        {
            printf("Product added successfully!\n");
            logAction("Added new product");
        }
        else
        {
            fprintf(stderr, "Failed to add product: %s\n", mysql_error(conn));
        }
    }

    mysql_free_result(result);
}

void updateOrderStatus()
{
    int orderId;
    char newStatus[20];

    printf("Enter Order ID to update: ");
    scanf("%d", &orderId);
    getchar(); // to consume newline

    printf("Enter new status (shipped/delivered): ");
    fgets(newStatus, sizeof(newStatus), stdin);
    newStatus[strcspn(newStatus, "\n")] = 0; // remove newline

    if (strcmp(newStatus, "shipped") != 0 && strcmp(newStatus, "delivered") != 0)
    {
        printf("Invalid status! Use 'shipped' or 'delivered'.\n");
        return;
    }

    char query[256];
    sprintf(query, "UPDATE orders SET status = '%s' WHERE id = %d", newStatus, orderId);

    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Failed to update order status: %s\n", mysql_error(conn));
        return;
    }

    if (mysql_affected_rows(conn) == 0)
    {
        printf("Error: No order found with ID %d.\n", orderId);
    }
    else
    {
        printf("Order status updated!\n");
        logAction("Updated order status");
    }
}

void viewCustomerOrders()
{
    char query[256];
    sprintf(query, "SELECT id, product_id, quantity, status FROM orders WHERE user_id=%d", loggedInUser);
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Failed to fetch orders: %s\n", mysql_error(conn));
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    printf("\nYour Orders:\n");
    while ((row = mysql_fetch_row(res)))
    {
        printf("Order ID: %s, Product ID: %s, Quantity: %s, Status: %s\n", row[0], row[1], row[2], row[3]);
    }
    mysql_free_result(res);
}

void processRefund(int orderId, float amount, const char *email)
{
    // Check if the logged-in user is an admin
    if (loggedInUser == -1 || strcmp(loggedInRole, "admin") != 0)
    {
        printf("Error: Only admins can process refunds!\n");
        return;
    }

    // Query to get the user ID associated with the given email and order ID
    char checkQuery[512];
    sprintf(checkQuery, "SELECT users.id FROM orders JOIN users ON orders.user_id = users.id WHERE orders.id = %d", orderId);

    if (mysql_query(conn, checkQuery))
    {
        fprintf(stderr, "Failed to verify order email: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);

    // Check if the order exists and verify that the provided email matches the customer who placed the order
    if (!row)
    {
        printf("Refund denied: No order found with ID %d.\n", orderId);
        mysql_free_result(res);
        return;
    }

    // Get the customer ID from the query result
    int customerId = atoi(row[0]);
    mysql_free_result(res);

    // Check if the provided email matches the customer's email
    char emailQuery[512];
    sprintf(emailQuery, "SELECT email FROM users WHERE id = %d", customerId);

    if (mysql_query(conn, emailQuery))
    {
        fprintf(stderr, "Failed to retrieve customer email: %s\n", mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);

    if (!row || strcmp(row[0], email) != 0)
    {
        printf("Refund denied: Provided email does not match the customer who placed the order.\n");
        mysql_free_result(res);
        return;
    }

    mysql_free_result(res);

    // Proceed with the refund, ensure that the refund is associated with the customer (not admin)
    char query[512];

    // Insert the refund record into the payments table with a negative amount (refund)
    sprintf(query, "INSERT INTO payments (order_id, user_id, amount, status) VALUES (%d, %d, %.2f, 'refunded')", orderId, customerId, -amount);

    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Refund failed: %s\n", mysql_error(conn));
    }
    else
    {
        printf("Refund processed successfully for Order ID %d!\n", orderId);
        logAction("Processed refund");
    }
}
int processPayment(int orderId, float amount)
{
    if (loggedInUser == -1)
    {
        printf("Error: User must be logged in to process payments!\n");
        return 0;
    }

    char query[512];

    printf("Simulating payment...\n");
    sleep(1); // Simulate delay

    int paymentSuccess = rand() % 2; // 50% chance

    if (paymentSuccess)
    {
        // Insert payment record
        sprintf(query, "INSERT INTO payments (order_id, user_id, amount, status) VALUES (%d, %d, %.2f, 'paid')",
                orderId, loggedInUser, amount);
        if (mysql_query(conn, query))
        {
            fprintf(stderr, "Failed to insert payment: %s\n", mysql_error(conn));
            return 0;
        }

        printf("Payment successful and order marked as paid.\n");
        logAction("Processed payment");
        return 1;
    }
    else
    {
        printf("Payment failed. Please try again.\n");
        logAction("Payment failed");
        return 0;
    }
}

void placeOrder(int productId, int quantity)
{
    char query[512];

    if (loggedInUser == -1)
    {
        printf("Please log in to place an order.\n");
        return;
    }

    // Start the transaction
    if (mysql_query(conn, "START TRANSACTION"))
    {
        fprintf(stderr, "Transaction start failed: %s\n", mysql_error(conn));
        return;
    }

    // Check product stock and price
    sprintf(query, "SELECT stock, price FROM products WHERE id=%d", productId);
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Stock check failed: %s\n", mysql_error(conn));
        mysql_query(conn, "ROLLBACK");
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        printf("Product not found!\n");
        mysql_free_result(res);
        mysql_query(conn, "ROLLBACK");
        return;
    }

    int stock = atoi(row[0]);
    double price = atof(row[1]);
    mysql_free_result(res);

    if (stock < quantity)
    {
        printf("Not enough stock available!\n");
        mysql_query(conn, "ROLLBACK");
        return;
    }

    double total = price * quantity;
    printf("Total amount: ₹%.2f\n", total);

    // Insert order first
    sprintf(query, "INSERT INTO orders (user_id, product_id, quantity, status) VALUES (%d, %d, %d, 'pending')",
            loggedInUser, productId, quantity);
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Order insertion failed: %s\n", mysql_error(conn));
        mysql_query(conn, "ROLLBACK");
        return;
    }

    // Get order ID for payment
    int order_id = mysql_insert_id(conn);

    // Process payment
    int success = processPayment(order_id, total);
    if (!success)
    {
        mysql_query(conn, "ROLLBACK");
        return;
    }

    // Update product stock
    sprintf(query, "UPDATE products SET stock = stock - %d WHERE id = %d", quantity, productId);
    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Stock update failed: %s\n", mysql_error(conn));
        mysql_query(conn, "ROLLBACK");
        return;
    }

    // Commit transaction
    if (mysql_query(conn, "COMMIT"))
    {
        fprintf(stderr, "Transaction commit failed: %s\n", mysql_error(conn));
        return;
    }

    printf("Order placed successfully!\n");
    logAction("Placed an order with successful payment");
}

void viewSalesReport()
{
    MYSQL_RES *result;
    MYSQL_ROW row;

    // SQL query to get the product sales data (quantity sold and total revenue for each product)
    const char *query =
        "SELECT p.name, SUM(o.quantity) AS total_quantity, SUM(o.quantity * p.price) AS total_revenue "
        "FROM orders o "
        "JOIN products p ON o.product_id = p.id "
        "GROUP BY p.id";

    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Failed to get sales data: %s\n", mysql_error(conn));
        return;
    }

    result = mysql_store_result(conn);

    if (result == NULL)
    {
        fprintf(stderr, "No data found: %s\n", mysql_error(conn));
        return;
    }

    printf("\n--- Sales Report ---\n");
    printf("Product Name | Total Quantity Sold | Total Revenue\n");

    double totalCost = 0.0;
    int totalQuantity = 0;

    // Process the query result and display the sales report
    while ((row = mysql_fetch_row(result)) != NULL)
    {
        char *productName = row[0];
        int quantitySold = atoi(row[1]);
        double revenue = atof(row[2]);

        totalQuantity += quantitySold;
        totalCost += revenue;

        printf("%-15s | %-19d | ₹%.2f\n", productName, quantitySold, revenue);
    }

    printf("\nTotal Quantity Sold: %d\n", totalQuantity);
    printf("Total Revenue: ₹%.2f\n", totalCost);

    mysql_free_result(result);

    logAction("Viewed sales report");
}

void deleteProduct()
{
    int productId;
    printf("Enter Product ID to delete: ");
    scanf("%d", &productId);

    char query[256];
    sprintf(query, "DELETE FROM products WHERE id=%d", productId);

    if (mysql_query(conn, query))
    {
        if (strstr(mysql_error(conn), "foreign key constraint fails"))
        {
            printf("Cannot delete this product because it has been ordered by a customer.\n");
        }
        else
        {
            fprintf(stderr, "Failed to delete product: %s\n", mysql_error(conn));
        }
    }
    else
    {
        printf("Product deleted successfully.\n");
        logAction("Deleted a product");
    }
}

void listPayments()
{
    char query[512];
    if (strcmp(loggedInRole, "admin") == 0)
    {
        // Admin can see all payments
        sprintf(query, "SELECT id, order_id, user_id, amount, status FROM payments");
    }
    else if (strcmp(loggedInRole, "customer") == 0)
    {
        // Customer can only see their own payments
        sprintf(query, "SELECT id, order_id, amount, status FROM payments WHERE user_id = %d", loggedInUser);
    }
    else
    {
        printf("Error: Unsupported role!\n");
        return;
    }

    if (mysql_query(conn, query))
    {
        fprintf(stderr, "Failed to retrieve payments: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    if (strcmp(loggedInRole, "admin") == 0)
    {
        printf("\nPayments List (Admin View):\n");
        printf("ID | Order ID | User ID | Amount | Status\n");
    }
    else
    {
        printf("\nYour Payments (Customer View):\n");
        printf("ID | Order ID | Amount | Status\n");
    }

    while ((row = mysql_fetch_row(res)))
    {
        if (strcmp(loggedInRole, "admin") == 0)
        {
            printf("%s | %s | %s | ₹%s | %s\n", row[0], row[1], row[2], row[3], row[4]);
        }
        else
        {
            printf("%s | %s | ₹%s | %s\n", row[0], row[1], row[2], row[3]);
        }
    }

    mysql_free_result(res);
}
void adminPanel()
{
    int choice;
    do
    {
        printf("\nAdmin Panel:\n");
        printf("1. List Users\n2. List Products\n3. List Orders\n4. Process Refund\n5. View Sales Report\n6. Add Product\n7. Update Order Status\n8. Delete Product\n9. List Payments\n10. Logout\nEnter choice: ");
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            listUsers();
            logAction("Viewed user list");
            break;
        case 2:
            listProducts();
            logAction("Viewed product list");
            break;
        case 3:
            listOrders();
            logAction("Viewed order list");
            break;
        case 4:
        {
            int orderId;
            float amount;
            char email[50];
            printf("Enter Order ID for refund: ");
            scanf("%d", &orderId);
            printf("Enter refund amount: ");
            scanf("%f", &amount);
            printf("Enter user email: ");
            scanf("%s", email);
            processRefund(orderId, amount, email);
            break;
        }
        case 5:
            viewSalesReport();
            break;
        case 6:
            addProduct();
            break;
        case 7:
            updateOrderStatus();
            break;
        case 8:
            deleteProduct();
            break;
        case 9:
            listPayments(); // Admin can view all payments
            break;
        case 10:
            logoutUser();
            return;
        default:
            printf("Invalid choice!\n");
        }
    } while (choice != 10);
}

void customerPanel()
{
    int choice;
    do
    {
        printf("\nCustomer Panel:\n");
        printf("1. Browse Products\n2. Place Order\n3. View Order History\n4. View Payments\n5. Logout\nEnter choice: ");
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            listProducts();
            break;
        case 2:
        {
            int productId, quantity;
            printf("Enter Product ID: ");
            scanf("%d", &productId);
            printf("Enter Quantity: ");
            scanf("%d", &quantity);
            placeOrder(productId, quantity);
            break;
        }
        case 3:
            viewCustomerOrders();
            break;
        case 4:
            listPayments(); // Customer can view their own payments
            break;
        case 5:
            logoutUser();
            return;
        default:
            printf("Invalid choice!\n");
        }
    } while (choice != 5);
}

int main()
{
    connectDatabase();
    char email[50], password[50];
    printf("Email: ");
    scanf("%s", email);
    printf("Password: ");
    scanf("%s", password);
    if (loginUser(email, password) != -1)
    {
        if (strcmp(loggedInRole, "admin") == 0)
            adminPanel();
        else if (strcmp(loggedInRole, "customer") == 0)
            customerPanel();
    }
    closeDatabase();
    return 0;
}
