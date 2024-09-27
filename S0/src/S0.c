// jeancarlo.saman@students.salle.url.edu - Jean Carlo Saman
// arnau.sf@students.salle.url.edu - Arnau Sanz Froiz
#define _GNU_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PRODUCTS_FILE "produtcs.bin"

void ess_print(const char * const string) {
    write(STDOUT_FILENO, string, strlen(string));
}
void ess_println(const char * const string) {
    ess_print(string);
    ess_print("\n");
}
void ess_print_error(const char * const string) {
    ess_print("\033[0;91mError: ");
    ess_println(string);
    ess_print("\033[0m");
}

char * ess_read_until(const int fd, const char end) {
    size_t buffer_size = 8;
    char *string = (char *) malloc(sizeof(char) * buffer_size);
    if (string == NULL) {
        ess_print_error("malloc failed.");
        return NULL;
    }

    char c = '\0';
    int i = 0;
    while (c != end) {
        const ssize_t size = read(fd, &c, sizeof(char));
        if (c == '\0') c = '-';
        //write(STDOUT_FILENO, &c, 1);
        if (size == 0) {
            if (i == 0) {
                free(string);
                return NULL;
            }
            break;
        }
        if (size < 0) {
            free(string);
            return NULL;
        }
        if(c != end){
            if (i + 1 >= buffer_size) {
                buffer_size = buffer_size * 2;
                char *temp = (char *) realloc(string, sizeof(char) * buffer_size);
                if (temp == NULL) {
                    free(string);
                    ess_print_error("realloc failed.");
                    return NULL;
                }
                string = temp;
            }
            string[i++] = c;
        }
    }
    string[i] = '\0';
    return string;
}
char * ess_read_line(const int fd) {
    return ess_read_until(fd, '\n');
}

struct Product {
    char name[100];
    char category[50];
    int max_stock;
    int current_stock;
    float unit_price;
    int supplier_id;
};

typedef struct {
    int id;
    char *supplierName;
    char *email;
    char *city;
    char *street;
} Supplier;

void print_product(const struct Product product) {
    ess_print("Product: ");
    ess_println(product.name);
    ess_print("\tCategory: ");
    ess_println(product.category);
    char * buffer;
    asprintf(&buffer, "\tMax stock: %d", product.max_stock);
    ess_println(buffer);
    free(buffer);
    asprintf(&buffer, "\tCurrent stock: %d", product.current_stock);
    ess_println(buffer);
    free(buffer);
    asprintf(&buffer, "\tUnit price: %.2f", product.unit_price);
    ess_println(buffer);
    free(buffer);
    asprintf(&buffer, "\tSupplied ID: %d", product.supplier_id);
}

void read_products(const int fd, struct Product **low_stock, struct Product **sufficient_stock, int *low_stock_amount, int *sufficient_amount) {
    *low_stock_amount = 1;
    *low_stock = (struct Product *) malloc(sizeof(struct Product) * *low_stock_amount);
    *sufficient_amount = 1;
    *sufficient_stock = (struct Product *) malloc(sizeof(struct Product) * *sufficient_amount);

    int low_stock_read = 0;
    int sufficient_read = 0;
    while (1) {
        struct Product temp_product;
        const ssize_t size = read(fd, &temp_product, sizeof(struct Product));
        if (size <= 0) {
            break;
        }
        //print_product(temp_product);
        if (temp_product.current_stock < 10) {
            //low_stock
            if (low_stock_read >= *low_stock_amount) {
                (*low_stock_amount)++;
                struct Product * temp_list = realloc(*low_stock, sizeof(struct Product) * *low_stock_amount);
                if (NULL == temp_list) {
                    free(*low_stock);
                    break;
                }
                *low_stock = temp_list;
            }
            (*low_stock)[low_stock_read++] = temp_product;
        }
        else {
            if (sufficient_read >= *sufficient_amount) {
                (*sufficient_amount)++;
                struct Product * temp_list = realloc(*sufficient_stock, sizeof(struct Product) * *sufficient_amount);
                if (NULL == temp_list) {
                    free(*sufficient_stock);
                    break;
                }
                *sufficient_stock = temp_list;
            }
            (*sufficient_stock)[sufficient_read++] = temp_product;
        }

    }
}


Supplier* read_suppliers(const char *suppliers_file, int *supplier_count) {
    const int fd_suppliers = open(suppliers_file, O_RDONLY);
    if (fd_suppliers < 0) {
        ess_print_error("Couldn't open suppliers file:");
        ess_print(suppliers_file);
        return NULL;
    }

    Supplier* suppliers = (Supplier*) malloc(sizeof(Supplier));
    if (suppliers == NULL) {
        ess_print_error("Error allocating memory for suppliers");
        exit(EXIT_FAILURE);
    }

    *supplier_count = 0; // Initialize the supplier count

    while (1) {
        char *line = ess_read_until(fd_suppliers, '&');
        if (line == NULL) {
            break;
        }

        int id = atoi(line);
        free(line);

        line = ess_read_until(fd_suppliers, '&');
        char *supplierName = line;
        line = ess_read_until(fd_suppliers, '&');
        char *email = line;
        line = ess_read_until(fd_suppliers, '&');
        char *city = line;
        line = ess_read_until(fd_suppliers, '\n');
        char *street = line;

        suppliers = (Supplier*) realloc(suppliers, sizeof(Supplier) * (*supplier_count + 1));
        if (suppliers == NULL) {
            ess_print_error("Error reallocating memory for suppliers");
            exit(EXIT_FAILURE);
        }

        suppliers[*supplier_count].id = id;
        suppliers[*supplier_count].supplierName = supplierName;
        suppliers[*supplier_count].email = email;
        suppliers[*supplier_count].city = city;
        suppliers[*supplier_count].street = street;

        (*supplier_count)++;
    }

    close(fd_suppliers);
    return suppliers;
}

int main (const int argc, char *argv[]) {

    if (argc != 3) {
        ess_print_error("Wrong number of arguments.");
        return 1;
    }

    const char * products_file = argv[1];
    const char * suppliers_file = argv[2];

    const int product_file = open(products_file, O_RDONLY);
    if (product_file < 0) {
        ess_print_error("Couldn't open product file:");
        ess_print(products_file);
        return 1;
    }

    struct Product *low_stock_products;
    struct Product *sufficient_products;

    int low_stock_product_amount;
    int sufficient_product_amount;

    read_products(product_file, &low_stock_products, &sufficient_products, &low_stock_product_amount, &sufficient_product_amount);

    /*int i = 0;
    for (i = 0; i < low_stock_product_amount; i++) {
        print_product(low_stock_products[i]);
    }
    for (i = 0; i < sufficient_product_amount; i++) {
        print_product(sufficient_products[i]);
    }*/

    const int current_stock_file = open("current_stock.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (current_stock_file < 0) {
        ess_print_error("Couldn't open demand file.");
        free(low_stock_products);
        free(sufficient_products);
        return 1;
    }

    for (int i = 0; i < sufficient_product_amount; i++) {
        char *buffer;
        asprintf(&buffer, "%s - Stock: %d - Category: %s\n", sufficient_products[i].name, sufficient_products[i].current_stock, sufficient_products[i].category);
        write(current_stock_file, buffer, strlen(buffer));
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);
    }

    int supplier_count = 0;

    Supplier* suppliers = read_suppliers(suppliers_file, &supplier_count);
    if (suppliers == NULL) {
        return 1;
    }

    const int output_file = open("demand.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output_file < 0) {
        ess_print_error("Couldn't open demand file.");
        free(suppliers);
        free(low_stock_products);
        free(sufficient_products);
        return 1;
    }

    int * products_listed = malloc(sizeof(int) * low_stock_product_amount);
    for (int j = 0; j < low_stock_product_amount; j++) {
        products_listed[j] = 0;
    }

    for (int i = 0; i < supplier_count; i++) {
        char *buffer;
        asprintf(&buffer, "%s (%s):\n", suppliers[i].supplierName, suppliers[i].email);
        write(output_file, buffer, strlen(buffer));
        write(STDOUT_FILENO, buffer, strlen(buffer));
        free(buffer);

        for (int j = 0; j < low_stock_product_amount; j++) {
            if (low_stock_products[j].supplier_id == suppliers[i].id) {
                if (products_listed[j] == 1) {
                    break;
                }
                asprintf(&buffer, "\t%s:\n", low_stock_products[j].category);
                write(output_file, buffer, strlen(buffer));
                write(STDOUT_FILENO, buffer, strlen(buffer));
                free(buffer);
                for (int k = j; k < low_stock_product_amount; k++) {
                    if (low_stock_products[k].supplier_id == suppliers[i].id && 0 == strcmp(low_stock_products[k].category, low_stock_products[j].category)) {
                        products_listed[k] = 1;
                        asprintf(&buffer, "\t\t- %s - Restock: %d\n", low_stock_products[k].name, (low_stock_products[k].max_stock - low_stock_products[k].current_stock));
                        //in the execution example it also shows the stock, if you need it it's easy to add ;)
                        write(output_file, buffer, strlen(buffer));
                        write(STDOUT_FILENO, buffer, strlen(buffer));
                        free(buffer);
                    }
                }
            }
        }
    }

    free(products_listed);
    close(output_file);

    for (int i = 0; i < supplier_count; i++) {
        free(suppliers[i].supplierName);
        free(suppliers[i].email);
        free(suppliers[i].city);
        free(suppliers[i].street);
    }
    free(suppliers);
    free(low_stock_products);
    free(sufficient_products);

    return 0;
}
