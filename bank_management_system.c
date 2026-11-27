/**
 * ============================================================================
 *                    BANK MANAGEMENT SYSTEM (ADS IN C)
 * ============================================================================
 * Course: Advanced Data Structures (ADS)
 * Language: C (ANSI C / C99 / C11 Standard Compliant)
 * 
 * CORE DATA STRUCTURES IMPLEMENTED & ARCHITECTURAL PURPOSE:
 * ----------------------------------------------------------------------------
 * 1. Hash Table (Separate Chaining Collision Resolution):
 *    - Purpose: Provides average O(1) instant account lookup by Account Number.
 *    - Mechanism: Bucket array with singly-linked list collision chains.
 *    - Analysis: Hash function h(k) = |k| % HASH_TABLE_SIZE.
 *
 * 2. AVL Tree (Self-Balancing Binary Search Tree):
 *    - Purpose: Organizes all accounts strictly ordered by Account Number.
 *    - Mechanism: Enforces balance factor BF in {-1, 0, +1} across all nodes.
 *      Performs LL, RR, LR, and RL rotations on insert and delete to ensure
 *      strictly guaranteed O(log N) search, insertion, and deletion time.
 *    - Inorder Traversal (Left -> Root -> Right): Naturally displays all
 *      accounts in strictly sorted numeric order in O(N) time.
 *    - Includes an ASCII hierarchical tree visualizer showing node heights & BF.
 *
 * 3. Singly Linked List / Queue (Transaction History / Passbook):
 *    - Purpose: Chronological ledger of all deposits, withdrawals, transfers,
 *      and loan payments per account.
 *    - Mechanism: Head & Tail pointers enable O(1) transaction logging.
 *
 * 4. Linked List (Loan Management Subsystem):
 *    - Purpose: Tracks active and repaid loans per account with interest,
 *      repayment schedules, terms, monthly EMI, and outstanding balances.
 *
 * 5. Persistent File Storage (No Database Software Required):
 *    - Purpose: All accounts, transactions, and loans are serialized to 
 *      'bank_records.txt' and automatically restored on startup.
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#define HASH_TABLE_SIZE     101
#define MAX_NAME_LEN        64
#define MAX_ADDR_LEN        128
#define MAX_PHONE_LEN       24
#define MAX_TYPE_LEN        16
#define DB_FILENAME         "bank_records.txt"

/* ============================================================================
 * DATA STRUCTURE DEFINITIONS
 * ============================================================================
 */

/* Forward declaration */
struct TransactionNode;
struct LoanNode;

/**
 * TransactionNode: Node representing an individual banking transaction.
 * Forms a chronological singly linked list (Passbook Statement) per account.
 */
typedef struct TransactionNode {
    long transaction_id;                    // Unique sequential transaction ID
    char timestamp[32];                     // Formatted date and time
    char type[32];                          // "OPENING", "DEPOSIT", "WITHDRAWAL", etc.
    double amount;                          // Transaction amount
    double balance_after;                   // Account balance after transaction
    struct TransactionNode* next;           // Link to next transaction
} TransactionNode;

/**
 * LoanNode: Node representing a loan application and record.
 * Form a linked list attached to the borrower's account.
 */
typedef struct LoanNode {
    int loan_id;                            // Unique sequential loan ID
    int account_number;                     // Associated customer account number
    double principal_amount;                // Sanctioned principal amount
    double interest_rate;                   // Annual interest rate percentage (e.g. 7.5%)
    int term_months;                        // Loan duration in months
    double total_due;                       // Principal + total calculated interest
    double outstanding_balance;             // Remaining unpaid balance
    char status[20];                        // "ACTIVE", "PAID_OFF"
    char applied_date[32];                  // Date & time loan was disbursed
    struct LoanNode* next;                  // Link to next loan record
} LoanNode;

/**
 * Account: Primary Bank Account Entity.
 * Referenced by both Hash Table buckets and AVL Tree nodes.
 */
typedef struct Account {
    int account_number;                     // Unique Primary Key
    char name[MAX_NAME_LEN];                // Customer Full Name
    char address[MAX_ADDR_LEN];             // Customer Postal/Residential Address
    char phone[MAX_PHONE_LEN];              // Customer Contact Phone Number
    char account_type[MAX_TYPE_LEN];        // "Savings" or "Current"
    double balance;                         // Current available balance
    
    // Transaction History (Linked List)
    TransactionNode* transactions_head;
    TransactionNode* transactions_tail;
    int transaction_count;

    // Associated Loans (Linked List)
    LoanNode* loans_head;
    int loan_count;
} Account;

/**
 * AVLNode: Node in the Self-Balancing Binary Search Tree.
 * Orders accounts by account_number with guaranteed O(log N) performance.
 */
typedef struct AVLNode {
    Account* account;                       // Pointer to the account record
    int height;                             // Height of node in AVL tree
    struct AVLNode* left;                   // Left child (< account_number)
    struct AVLNode* right;                  // Right child (> account_number)
} AVLNode;

/**
 * HashNode: Node in the Hash Table separate chaining bucket list.
 */
typedef struct HashNode {
    Account* account;                       // Pointer to the account record
    struct HashNode* next;                  // Link for collision chain
} HashNode;

/**
 * HashTable: Hash Table structure with bucket array and element counter.
 */
typedef struct HashTable {
    HashNode* buckets[HASH_TABLE_SIZE];
    int total_accounts;
} HashTable;

/* ============================================================================
 * GLOBAL SYSTEM VARIABLES
 * ============================================================================
 */
static AVLNode*    g_avl_root             = NULL;
static HashTable   g_hash_table;
static long        g_next_transaction_id  = 10001;
static int         g_next_loan_id         = 5001;

/* ============================================================================
 * UTILITY & INPUT HELPER FUNCTIONS
 * ============================================================================
 */

/**
 * Clears trailing characters and newline from standard input buffer.
 */
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

/**
 * Removes trailing newline or carriage return characters from a string.
 */
void trim_newline(char* str) {
    if (!str) return;
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

/**
 * Safely reads a non-empty string line from standard input.
 */
bool safe_read_line(char* buffer, size_t max_size, const char* prompt) {
    printf("%s", prompt);
    if (!fgets(buffer, (int)max_size, stdin)) {
        return false;
    }
    trim_newline(buffer);
    return (strlen(buffer) > 0);
}

/**
 * Safely reads an integer value from user with validation.
 */
bool safe_read_int(int* output, const char* prompt) {
    char buf[64];
    printf("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) {
        return false;
    }
    trim_newline(buf);
    if (strlen(buf) == 0) return false;

    char* endptr;
    long val = strtol(buf, &endptr, 10);
    if (*endptr != '\0') {
        return false;
    }
    *output = (int)val;
    return true;
}

/**
 * Safely reads a positive double value from user with validation.
 */
bool safe_read_double(double* output, const char* prompt) {
    char buf[64];
    printf("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) {
        return false;
    }
    trim_newline(buf);
    if (strlen(buf) == 0) return false;

    char* endptr;
    double val = strtod(buf, &endptr);
    if (*endptr != '\0') {
        return false;
    }
    *output = val;
    return true;
}

/**
 * Generates a formatted current timestamp string: "YYYY-MM-DD HH:MM:SS"
 */
void get_current_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    if (t) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
    } else {
        snprintf(buffer, size, "2026-11-12 12:00:00");
    }
}

/**
 * Returns the maximum of two integers.
 */
int get_max(int a, int b) {
    return (a > b) ? a : b;
}

/* ============================================================================
 * 1. TRANSACTION LINKED LIST IMPLEMENTATION
 * ============================================================================
 */

/**
 * Appends a new transaction node to the end of the account's transaction queue/list.
 * Time Complexity: O(1) via tail pointer.
 */
void record_transaction(Account* acc, const char* type, double amount) {
    if (!acc) return;

    TransactionNode* node = (TransactionNode*)malloc(sizeof(TransactionNode));
    if (!node) {
        printf("[Error] Memory allocation failed for transaction log.\n");
        return;
    }

    node->transaction_id = g_next_transaction_id++;
    strncpy(node->type, type, sizeof(node->type) - 1);
    node->type[sizeof(node->type) - 1] = '\0';
    node->amount = amount;
    node->balance_after = acc->balance;
    get_current_timestamp(node->timestamp, sizeof(node->timestamp));
    node->next = NULL;

    if (acc->transactions_tail == NULL) {
        acc->transactions_head = node;
        acc->transactions_tail = node;
    } else {
        acc->transactions_tail->next = node;
        acc->transactions_tail = node;
    }
    acc->transaction_count++;
}

/**
 * Appends an existing transaction node (loaded from file) to the account's list.
 */
void append_loaded_transaction(Account* acc, long tx_id, const char* timestamp, 
                               const char* type, double amount, double balance_after) {
    if (!acc) return;

    TransactionNode* node = (TransactionNode*)malloc(sizeof(TransactionNode));
    if (!node) return;

    node->transaction_id = tx_id;
    strncpy(node->timestamp, timestamp, sizeof(node->timestamp) - 1);
    node->timestamp[sizeof(node->timestamp) - 1] = '\0';
    strncpy(node->type, type, sizeof(node->type) - 1);
    node->type[sizeof(node->type) - 1] = '\0';
    node->amount = amount;
    node->balance_after = balance_after;
    node->next = NULL;

    if (acc->transactions_tail == NULL) {
        acc->transactions_head = node;
        acc->transactions_tail = node;
    } else {
        acc->transactions_tail->next = node;
        acc->transactions_tail = node;
    }
    acc->transaction_count++;

    if (tx_id >= g_next_transaction_id) {
        g_next_transaction_id = tx_id + 1;
    }
}

/**
 * Traverses and displays the transaction history (Passbook Statement) of an account.
 * Time Complexity: O(T) where T = number of transactions.
 */
void display_transaction_history(const Account* acc) {
    if (!acc) {
        printf("[Error] Invalid account record.\n");
        return;
    }

    printf("\n========================================================================================\n");
    printf("                  TRANSACTION STATEMENT / PASSBOOK : ACCOUNT #%d\n", acc->account_number);
    printf("  Customer Name: %-28s Phone: %-15s\n", acc->name, acc->phone);
    printf("  Account Type : %-28s Balance: $%.2f\n", acc->account_type, acc->balance);
    printf("========================================================================================\n");
    printf("%-10s | %-19s | %-18s | %-12s | %-14s\n", 
           "TX ID", "DATE & TIME", "TRANSACTION TYPE", "AMOUNT ($)", "BALANCE ($)");
    printf("----------------------------------------------------------------------------------------\n");

    TransactionNode* cur = acc->transactions_head;
    if (!cur) {
        printf("                      No transaction history recorded yet.\n");
    } else {
        while (cur) {
            printf("%-10ld | %-19s | %-18s | %12.2f | %14.2f\n",
                   cur->transaction_id, cur->timestamp, cur->type, cur->amount, cur->balance_after);
            cur = cur->next;
        }
    }
    printf("========================================================================================\n");
    printf("Total Transactions: %d\n\n", acc->transaction_count);
}

/**
 * Frees all dynamically allocated nodes in the transaction linked list.
 */
void free_transactions(TransactionNode* head) {
    TransactionNode* cur = head;
    while (cur) {
        TransactionNode* next = cur->next;
        free(cur);
        cur = next;
    }
}

/* ============================================================================
 * 2. LOAN MANAGEMENT IMPLEMENTATION
 * ============================================================================
 */

/**
 * Creates and appends a loan record to an account's loan linked list.
 */
LoanNode* add_loan_record(Account* acc, double principal, double rate, int term_months) {
    if (!acc) return NULL;

    LoanNode* loan = (LoanNode*)malloc(sizeof(LoanNode));
    if (!loan) {
        printf("[Error] Memory allocation failed for loan record.\n");
        return NULL;
    }

    loan->loan_id = g_next_loan_id++;
    loan->account_number = acc->account_number;
    loan->principal_amount = principal;
    loan->interest_rate = rate;
    loan->term_months = term_months;

    // Simple Interest Formula: Interest = Principal * (Rate / 100) * (Months / 12)
    double interest = principal * (rate / 100.0) * ((double)term_months / 12.0);
    loan->total_due = principal + interest;
    loan->outstanding_balance = loan->total_due;

    strncpy(loan->status, "ACTIVE", sizeof(loan->status) - 1);
    loan->status[sizeof(loan->status) - 1] = '\0';
    get_current_timestamp(loan->applied_date, sizeof(loan->applied_date));

    // Insert at front of account's loan linked list
    loan->next = acc->loans_head;
    acc->loans_head = loan;
    acc->loan_count++;

    return loan;
}

/**
 * Appends an existing loan node loaded from file.
 */
void append_loaded_loan(Account* acc, int loan_id, double principal, double rate, 
                        int term, double total_due, double balance, 
                        const char* status, const char* applied_date) {
    if (!acc) return;

    LoanNode* loan = (LoanNode*)malloc(sizeof(LoanNode));
    if (!loan) return;

    loan->loan_id = loan_id;
    loan->account_number = acc->account_number;
    loan->principal_amount = principal;
    loan->interest_rate = rate;
    loan->term_months = term;
    loan->total_due = total_due;
    loan->outstanding_balance = balance;
    strncpy(loan->status, status, sizeof(loan->status) - 1);
    loan->status[sizeof(loan->status) - 1] = '\0';
    strncpy(loan->applied_date, applied_date, sizeof(loan->applied_date) - 1);
    loan->applied_date[sizeof(loan->applied_date) - 1] = '\0';

    // Insert at front
    loan->next = acc->loans_head;
    acc->loans_head = loan;
    acc->loan_count++;

    if (loan_id >= g_next_loan_id) {
        g_next_loan_id = loan_id + 1;
    }
}

/**
 * Searches for a specific loan by ID within an account's loan list.
 */
LoanNode* find_account_loan(Account* acc, int loan_id) {
    if (!acc) return NULL;
    LoanNode* cur = acc->loans_head;
    while (cur) {
        if (cur->loan_id == loan_id) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

/**
 * Displays all loans associated with an account.
 */
void display_account_loans(const Account* acc) {
    if (!acc) return;

    printf("\n================================================================================================\n");
    printf("                       LOAN PORTFOLIO : ACCOUNT #%d (%s)\n", acc->account_number, acc->name);
    printf("================================================================================================\n");
    printf("%-8s | %-12s | %-8s | %-6s | %-12s | %-14s | %-10s\n",
           "LOAN ID", "PRINCIPAL($)", "RATE(%)", "TERM", "TOTAL DUE($)", "REMAINING($)", "STATUS");
    printf("------------------------------------------------------------------------------------------------\n");

    LoanNode* cur = acc->loans_head;
    if (!cur) {
        printf("                            No loan history found for this account.\n");
    } else {
        while (cur) {
            printf("%-8d | %12.2f | %7.2f%% | %3dm  | %12.2f | %14.2f | %-10s\n",
                   cur->loan_id, cur->principal_amount, cur->interest_rate, cur->term_months,
                   cur->total_due, cur->outstanding_balance, cur->status);
            cur = cur->next;
        }
    }
    printf("================================================================================================\n\n");
}

/**
 * Frees all loan nodes associated with an account.
 */
void free_loans(LoanNode* head) {
    LoanNode* cur = head;
    while (cur) {
        LoanNode* next = cur->next;
        free(cur);
        cur = next;
    }
}

/* ============================================================================
 * 3. HASH TABLE IMPLEMENTATION (Separate Chaining for Fast O(1) Lookup)
 * ============================================================================
 */

/**
 * Initializes all hash table buckets to NULL.
 */
void hash_init(HashTable* ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        ht->buckets[i] = NULL;
    }
    ht->total_accounts = 0;
}

/**
 * Primary hash function: Simple modulo hashing on positive account numbers.
 * Time Complexity: O(1).
 */
unsigned int hash_code(int account_number) {
    if (account_number < 0) account_number = -account_number;
    return (unsigned int)account_number % HASH_TABLE_SIZE;
}

/**
 * Inserts an account reference into the hash table.
 * Resolves collisions using separate chaining at the head of the bucket.
 * Time Complexity: Average O(1).
 */
bool hash_insert(HashTable* ht, Account* acc) {
    if (!ht || !acc) return false;

    unsigned int index = hash_code(acc->account_number);

    // Verify uniqueness
    HashNode* cur = ht->buckets[index];
    while (cur) {
        if (cur->account->account_number == acc->account_number) {
            return false; // Duplicate found
        }
        cur = cur->next;
    }

    HashNode* node = (HashNode*)malloc(sizeof(HashNode));
    if (!node) {
        printf("[Error] Memory allocation failed for Hash Table node.\n");
        return false;
    }

    node->account = acc;
    node->next = ht->buckets[index];
    ht->buckets[index] = node;
    ht->total_accounts++;
    return true;
}

/**
 * Searches for an account by its unique account number.
 * Time Complexity: Average O(1), Worst O(N) if extreme collisions occur.
 */
Account* hash_search(const HashTable* ht, int account_number) {
    if (!ht) return NULL;
    unsigned int index = hash_code(account_number);
    HashNode* cur = ht->buckets[index];
    while (cur) {
        if (cur->account->account_number == account_number) {
            return cur->account;
        }
        cur = cur->next;
    }
    return NULL;
}

/**
 * Deletes an account entry from the hash table bucket list.
 * Time Complexity: Average O(1).
 */
bool hash_delete(HashTable* ht, int account_number) {
    if (!ht) return false;
    unsigned int index = hash_code(account_number);
    HashNode* cur = ht->buckets[index];
    HashNode* prev = NULL;

    while (cur) {
        if (cur->account->account_number == account_number) {
            if (prev == NULL) {
                ht->buckets[index] = cur->next;
            } else {
                prev->next = cur->next;
            }
            free(cur);
            ht->total_accounts--;
            return true;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}

/**
 * Computes and prints Hash Table performance metrics (load factor, collisions).
 */
void hash_display_metrics(const HashTable* ht) {
    if (!ht) return;

    int occupied_buckets = 0;
    int collisions = 0;
    int max_chain = 0;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        int chain_len = 0;
        HashNode* cur = ht->buckets[i];
        if (cur) occupied_buckets++;

        while (cur) {
            chain_len++;
            cur = cur->next;
        }
        if (chain_len > 1) {
            collisions += (chain_len - 1);
        }
        if (chain_len > max_chain) {
            max_chain = chain_len;
        }
    }

    double load_factor = (double)ht->total_accounts / (double)HASH_TABLE_SIZE;

    printf("\n========================================================\n");
    printf("             HASH TABLE DIAGNOSTICS & METRICS           \n");
    printf("========================================================\n");
    printf("  Bucket Array Size   : %d\n", HASH_TABLE_SIZE);
    printf("  Total Accounts      : %d\n", ht->total_accounts);
    printf("  Occupied Buckets    : %d (%.1f%%)\n", 
           occupied_buckets, (double)occupied_buckets * 100.0 / HASH_TABLE_SIZE);
    printf("  Empty Buckets       : %d\n", HASH_TABLE_SIZE - occupied_buckets);
    printf("  Total Collisions    : %d\n", collisions);
    printf("  Longest Bucket Chain: %d\n", max_chain);
    printf("  Load Factor (Alpha) : %.3f\n", load_factor);
    printf("  Average Lookup Time : O(1)\n");
    printf("========================================================\n\n");
}

/**
 * Frees all hash chain nodes (does NOT free the Account records themselves).
 */
void hash_free_buckets(HashTable* ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode* cur = ht->buckets[i];
        while (cur) {
            HashNode* next = cur->next;
            free(cur);
            cur = next;
        }
        ht->buckets[i] = NULL;
    }
    ht->total_accounts = 0;
}

/* ============================================================================
 * 4. AVL TREE IMPLEMENTATION (Self-Balancing BST for Ordering Accounts)
 * ============================================================================
 */

/**
 * Returns height of node; NULL nodes have height 0.
 */
int avl_node_height(AVLNode* node) {
    return node ? node->height : 0;
}

/**
 * Calculates Balance Factor = Height(Left) - Height(Right).
 */
int avl_balance_factor(AVLNode* node) {
    return node ? (avl_node_height(node->left) - avl_node_height(node->right)) : 0;
}

/**
 * Updates height based on the max height of its left and right subtrees.
 */
void avl_update_height(AVLNode* node) {
    if (node) {
        node->height = 1 + get_max(avl_node_height(node->left), avl_node_height(node->right));
    }
}

/**
 * Right Rotation (RR): Used for Left-Left (LL) imbalance.
 * Time Complexity: O(1).
 */
AVLNode* avl_rotate_right(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* t2 = x->right;

    x->right = y;
    y->left = t2;

    avl_update_height(y);
    avl_update_height(x);

    return x;
}

/**
 * Left Rotation (LL): Used for Right-Right (RR) imbalance.
 * Time Complexity: O(1).
 */
AVLNode* avl_rotate_left(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* t2 = y->left;

    y->left = x;
    x->right = t2;

    avl_update_height(x);
    avl_update_height(y);

    return y;
}

/**
 * Inserts an Account pointer into the AVL Tree strictly sorted by account_number.
 * Automatically restores balance via rotations if balance factor falls outside [-1, +1].
 * Time Complexity: Strictly O(log N).
 */
AVLNode* avl_insert(AVLNode* root, Account* acc, bool* success) {
    if (root == NULL) {
        AVLNode* node = (AVLNode*)malloc(sizeof(AVLNode));
        if (!node) {
            printf("[Error] Memory allocation failed for AVL Tree node.\n");
            *success = false;
            return NULL;
        }
        node->account = acc;
        node->height = 1;
        node->left = NULL;
        node->right = NULL;
        *success = true;
        return node;
    }

    if (acc->account_number < root->account->account_number) {
        root->left = avl_insert(root->left, acc, success);
    } else if (acc->account_number > root->account->account_number) {
        root->right = avl_insert(root->right, acc, success);
    } else {
        // Duplicate account number not permitted
        *success = false;
        return root;
    }

    if (!(*success)) return root;

    // 1. Update height of current ancestor node
    avl_update_height(root);

    // 2. Obtain balance factor
    int balance = avl_balance_factor(root);

    // 3. Rebalance if needed (4 cases)

    // Case 1: Left-Left (LL)
    if (balance > 1 && acc->account_number < root->left->account->account_number) {
        return avl_rotate_right(root);
    }

    // Case 2: Right-Right (RR)
    if (balance < -1 && acc->account_number > root->right->account->account_number) {
        return avl_rotate_left(root);
    }

    // Case 3: Left-Right (LR)
    if (balance > 1 && acc->account_number > root->left->account->account_number) {
        root->left = avl_rotate_left(root->left);
        return avl_rotate_right(root);
    }

    // Case 4: Right-Left (RL)
    if (balance < -1 && acc->account_number < root->right->account->account_number) {
        root->right = avl_rotate_right(root->right);
        return avl_rotate_left(root);
    }

    return root;
}

/**
 * Finds the node with the minimum account number in a subtree.
 */
AVLNode* avl_min_value_node(AVLNode* node) {
    AVLNode* cur = node;
    while (cur && cur->left != NULL) {
        cur = cur->left;
    }
    return cur;
}

/**
 * Deletes an account node from the AVL Tree by account_number and rebalances.
 * Time Complexity: Strictly O(log N).
 */
AVLNode* avl_delete(AVLNode* root, int account_number, bool* success) {
    if (root == NULL) {
        *success = false;
        return NULL;
    }

    if (account_number < root->account->account_number) {
        root->left = avl_delete(root->left, account_number, success);
    } else if (account_number > root->account->account_number) {
        root->right = avl_delete(root->right, account_number, success);
    } else {
        // Node found
        *success = true;

        // Node with 0 or 1 child
        if (root->left == NULL || root->right == NULL) {
            AVLNode* temp = root->left ? root->left : root->right;
            if (temp == NULL) {
                // No child
                free(root);
                return NULL;
            } else {
                // One child
                AVLNode* old = root;
                root = temp;
                free(old);
            }
        } else {
            // Node with 2 children: get inorder successor (smallest in right subtree)
            AVLNode* temp = avl_min_value_node(root->right);
            root->account = temp->account;
            bool dummy = false;
            root->right = avl_delete(root->right, temp->account->account_number, &dummy);
        }
    }

    if (root == NULL) return NULL;

    // Update height
    avl_update_height(root);

    // Check balance factor
    int balance = avl_balance_factor(root);

    // Left-Left
    if (balance > 1 && avl_balance_factor(root->left) >= 0) {
        return avl_rotate_right(root);
    }
    // Left-Right
    if (balance > 1 && avl_balance_factor(root->left) < 0) {
        root->left = avl_rotate_left(root->left);
        return avl_rotate_right(root);
    }
    // Right-Right
    if (balance < -1 && avl_balance_factor(root->right) <= 0) {
        return avl_rotate_left(root);
    }
    // Right-Left
    if (balance < -1 && avl_balance_factor(root->right) > 0) {
        root->right = avl_rotate_right(root->right);
        return avl_rotate_left(root);
    }

    return root;
}

/**
 * Searches for an account in the AVL Tree.
 * Time Complexity: Strictly O(log N).
 */
Account* avl_search(AVLNode* root, int account_number) {
    if (root == NULL) return NULL;
    if (account_number == root->account->account_number) return root->account;
    if (account_number < root->account->account_number) return avl_search(root->left, account_number);
    return avl_search(root->right, account_number);
}

/**
 * Recursively performs Inorder Traversal (Left -> Root -> Right) of the AVL Tree.
 * Displays accounts strictly sorted by account_number and accumulates metrics.
 * Time Complexity: O(N).
 */
void avl_inorder_traverse(AVLNode* root, int* count, double* total_deposits, double* total_loans) {
    if (root == NULL) return;

    avl_inorder_traverse(root->left, count, total_deposits, total_loans);

    Account* acc = root->account;
    (*count)++;
    *total_deposits += acc->balance;

    double acc_loan_sum = 0.0;
    LoanNode* l = acc->loans_head;
    while (l) {
        if (strcmp(l->status, "ACTIVE") == 0) {
            acc_loan_sum += l->outstanding_balance;
        }
        l = l->next;
    }
    *total_loans += acc_loan_sum;

    printf("%-9d | %-20s | %-9s | %-13s | %12.2f | %12.2f | %-4d\n",
           acc->account_number, acc->name, acc->account_type, acc->phone,
           acc->balance, acc_loan_sum, acc->transaction_count);

    avl_inorder_traverse(root->right, count, total_deposits, total_loans);
}

/**
 * Visual ASCII Tree Printer for showing AVL balance factor and height.
 */
void avl_print_visual(AVLNode* root, int space) {
    if (root == NULL) return;
    space += 8;
    avl_print_visual(root->right, space);
    printf("\n");
    for (int i = 8; i < space; i++) printf(" ");
    printf("[#%d | H:%d | BF:%d]\n", 
           root->account->account_number, root->height, avl_balance_factor(root));
    avl_print_visual(root->left, space);
}

/**
 * Frees AVL tree nodes without freeing the underlying Account structs.
 */
void avl_free_tree(AVLNode* root) {
    if (root == NULL) return;
    avl_free_tree(root->left);
    avl_free_tree(root->right);
    free(root);
}

/* ============================================================================
 * 5. CORE ENTITY LIFECYCLE & INTEGRATED ADS REGISTRATION
 * ============================================================================
 */

/**
 * Dynamically allocates a new Bank Account structure with initialized fields.
 */
Account* create_account_record(int acc_no, const char* name, const char* address,
                               const char* phone, const char* acc_type, double opening_balance) {
    Account* acc = (Account*)malloc(sizeof(Account));
    if (!acc) {
        printf("[Error] Memory allocation failed for Account.\n");
        return NULL;
    }

    acc->account_number = acc_no;
    strncpy(acc->name, name, sizeof(acc->name) - 1);
    acc->name[sizeof(acc->name) - 1] = '\0';
    strncpy(acc->address, address, sizeof(acc->address) - 1);
    acc->address[sizeof(acc->address) - 1] = '\0';
    strncpy(acc->phone, phone, sizeof(acc->phone) - 1);
    acc->phone[sizeof(acc->phone) - 1] = '\0';
    strncpy(acc->account_type, acc_type, sizeof(acc->account_type) - 1);
    acc->account_type[sizeof(acc->account_type) - 1] = '\0';
    acc->balance = opening_balance;

    // Sanitize any delimiter '|' characters in text fields to prevent file format corruption
    for (size_t i = 0; i < strlen(acc->name); i++) {
        if (acc->name[i] == '|') acc->name[i] = ' ';
    }
    for (size_t i = 0; i < strlen(acc->address); i++) {
        if (acc->address[i] == '|') acc->address[i] = ' ';
    }

    acc->transactions_head = NULL;
    acc->transactions_tail = NULL;
    acc->transaction_count = 0;

    acc->loans_head = NULL;
    acc->loan_count = 0;

    return acc;
}

/**
 * Registers an account in both ADS structures (AVL Tree and Hash Table).
 */
bool register_account_in_system(Account* acc) {
    if (!acc) return false;

    // Check duplicate in Hash Table first (O(1))
    if (hash_search(&g_hash_table, acc->account_number) != NULL) {
        return false;
    }

    bool avl_success = false;
    g_avl_root = avl_insert(g_avl_root, acc, &avl_success);
    if (!avl_success) {
        return false;
    }

    hash_insert(&g_hash_table, acc);
    return true;
}

/**
 * Deletes an account from both AVL Tree and Hash Table, then deallocates all memory.
 */
bool delete_account_from_system(int account_number) {
    Account* acc = hash_search(&g_hash_table, account_number);
    if (!acc) return false;

    // Check for active loans
    LoanNode* l = acc->loans_head;
    while (l) {
        if (strcmp(l->status, "ACTIVE") == 0 && l->outstanding_balance > 0.01) {
            printf("[Warning] Cannot delete account #%d: It has an active loan with outstanding balance $%.2f.\n",
                   account_number, l->outstanding_balance);
            return false;
        }
        l = l->next;
    }

    // 1. Remove from Hash Table
    hash_delete(&g_hash_table, account_number);

    // 2. Remove from AVL Tree
    bool avl_deleted = false;
    g_avl_root = avl_delete(g_avl_root, account_number, &avl_deleted);

    // 3. Free transactions and loans
    free_transactions(acc->transactions_head);
    free_loans(acc->loans_head);

    // 4. Free account struct
    free(acc);
    return true;
}

/**
 * Recursively deallocates all accounts, transaction lists, and loan lists.
 */
void free_all_accounts_recursive(AVLNode* root) {
    if (!root) return;
    free_all_accounts_recursive(root->left);
    free_all_accounts_recursive(root->right);

    if (root->account) {
        free_transactions(root->account->transactions_head);
        free_loans(root->account->loans_head);
        free(root->account);
        root->account = NULL;
    }
}

/**
 * Complete teardown of all system memory.
 */
void system_cleanup(void) {
    free_all_accounts_recursive(g_avl_root);
    avl_free_tree(g_avl_root);
    g_avl_root = NULL;
    hash_free_buckets(&g_hash_table);
}

/* ============================================================================
 * 6. PERSISTENT FILE STORAGE ENGINE (SAVE & LOAD)
 * ============================================================================
 */

/**
 * Recursive helper to serialize all accounts in sorted order to the file.
 */
void save_accounts_to_file(FILE* fp, AVLNode* root) {
    if (root == NULL) return;
    save_accounts_to_file(fp, root->left);

    Account* acc = root->account;
    // Format: AccNo|Name|Address|Phone|AccountType|Balance
    fprintf(fp, "%d|%s|%s|%s|%s|%.2f\n",
            acc->account_number, acc->name, acc->address, acc->phone, 
            acc->account_type, acc->balance);

    save_accounts_to_file(fp, root->right);
}

/**
 * Recursive helper to serialize all transaction logs to the file.
 */
void save_transactions_to_file(FILE* fp, AVLNode* root) {
    if (root == NULL) return;
    save_transactions_to_file(fp, root->left);

    Account* acc = root->account;
    TransactionNode* tx = acc->transactions_head;
    while (tx) {
        // Format: TxID|AccNo|Timestamp|Type|Amount|BalanceAfter
        fprintf(fp, "%ld|%d|%s|%s|%.2f|%.2f\n",
                tx->transaction_id, acc->account_number, tx->timestamp, 
                tx->type, tx->amount, tx->balance_after);
        tx = tx->next;
    }

    save_transactions_to_file(fp, root->right);
}

/**
 * Recursive helper to serialize all loan records to the file.
 */
void save_loans_to_file(FILE* fp, AVLNode* root) {
    if (root == NULL) return;
    save_loans_to_file(fp, root->left);

    Account* acc = root->account;
    LoanNode* l = acc->loans_head;
    while (l) {
        // Format: LoanID|AccNo|Principal|Rate|Term|TotalDue|Balance|Status|AppliedDate
        fprintf(fp, "%d|%d|%.2f|%.2f|%d|%.2f|%.2f|%s|%s\n",
                l->loan_id, l->account_number, l->principal_amount, 
                l->interest_rate, l->term_months, l->total_due, 
                l->outstanding_balance, l->status, l->applied_date);
        l = l->next;
    }

    save_loans_to_file(fp, root->right);
}

/**
 * Saves all accounts, transactions, and loans to the disk file.
 */
bool save_database(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("[Error] Cannot open file '%s' for saving records.\n", filename);
        return false;
    }

    fprintf(fp, "# BANK MANAGEMENT SYSTEM DATABASE FILE\n");
    
    // 1. Accounts section
    fprintf(fp, "[ACCOUNTS]\n");
    save_accounts_to_file(fp, g_avl_root);

    // 2. Transactions section
    fprintf(fp, "[TRANSACTIONS]\n");
    save_transactions_to_file(fp, g_avl_root);

    // 3. Loans section
    fprintf(fp, "[LOANS]\n");
    save_loans_to_file(fp, g_avl_root);

    fprintf(fp, "[EOF]\n");
    fclose(fp);
    return true;
}

/**
 * Loads and reconstructs the complete system database from the disk file.
 */
bool load_database(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("[Info] Database file '%s' not found. Starting with a fresh system.\n", filename);
        return false;
    }

    char line[512];
    int section = 0; // 0: None, 1: Accounts, 2: Transactions, 3: Loans
    int acc_loaded = 0;
    int tx_loaded = 0;
    int loan_loaded = 0;

    while (fgets(line, sizeof(line), fp)) {
        trim_newline(line);
        if (strlen(line) == 0 || line[0] == '#') continue;

        if (strcmp(line, "[ACCOUNTS]") == 0) {
            section = 1;
            continue;
        } else if (strcmp(line, "[TRANSACTIONS]") == 0) {
            section = 2;
            continue;
        } else if (strcmp(line, "[LOANS]") == 0) {
            section = 3;
            continue;
        } else if (strcmp(line, "[EOF]") == 0) {
            break;
        }

        if (section == 1) {
            // AccNo|Name|Address|Phone|AccountType|Balance
            char* token = strtok(line, "|");
            if (!token) continue;
            int acc_no = atoi(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            char name[MAX_NAME_LEN];
            strncpy(name, token, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';

            token = strtok(NULL, "|");
            if (!token) continue;
            char address[MAX_ADDR_LEN];
            strncpy(address, token, sizeof(address) - 1);
            address[sizeof(address) - 1] = '\0';

            token = strtok(NULL, "|");
            if (!token) continue;
            char phone[MAX_PHONE_LEN];
            strncpy(phone, token, sizeof(phone) - 1);
            phone[sizeof(phone) - 1] = '\0';

            token = strtok(NULL, "|");
            if (!token) continue;
            char acc_type[MAX_TYPE_LEN];
            strncpy(acc_type, token, sizeof(acc_type) - 1);
            acc_type[sizeof(acc_type) - 1] = '\0';

            token = strtok(NULL, "|");
            if (!token) continue;
            double balance = atof(token);

            Account* acc = create_account_record(acc_no, name, address, phone, acc_type, balance);
            if (acc && register_account_in_system(acc)) {
                acc_loaded++;
            } else {
                if (acc) free(acc);
            }
        } else if (section == 2) {
            // TxID|AccNo|Timestamp|Type|Amount|BalanceAfter
            char* token = strtok(line, "|");
            if (!token) continue;
            long tx_id = atol(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            int acc_no = atoi(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            char ts[32];
            strncpy(ts, token, sizeof(ts) - 1);
            ts[sizeof(ts) - 1] = '\0';

            token = strtok(NULL, "|");
            if (!token) continue;
            char type[32];
            strncpy(type, token, sizeof(type) - 1);
            type[sizeof(type) - 1] = '\0';

            token = strtok(NULL, "|");
            if (!token) continue;
            double amount = atof(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            double balance_after = atof(token);

            Account* acc = hash_search(&g_hash_table, acc_no);
            if (acc) {
                append_loaded_transaction(acc, tx_id, ts, type, amount, balance_after);
                tx_loaded++;
            }
        } else if (section == 3) {
            // LoanID|AccNo|Principal|Rate|Term|TotalDue|Balance|Status|AppliedDate
            char* token = strtok(line, "|");
            if (!token) continue;
            int loan_id = atoi(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            int acc_no = atoi(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            double principal = atof(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            double rate = atof(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            int term = atoi(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            double total_due = atof(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            double balance = atof(token);

            token = strtok(NULL, "|");
            if (!token) continue;
            char status[20];
            strncpy(status, token, sizeof(status) - 1);
            status[sizeof(status) - 1] = '\0';

            token = strtok(NULL, "|");
            if (!token) continue;
            char applied_date[32];
            strncpy(applied_date, token, sizeof(applied_date) - 1);
            applied_date[sizeof(applied_date) - 1] = '\0';

            Account* acc = hash_search(&g_hash_table, acc_no);
            if (acc) {
                append_loaded_loan(acc, loan_id, principal, rate, term, total_due, balance, status, applied_date);
                loan_loaded++;
            }
        }
    }

    fclose(fp);
    printf("[Success] Restored %d accounts, %d transactions, and %d loans from '%s'.\n",
           acc_loaded, tx_loaded, loan_loaded, filename);
    return true;
}

/* ============================================================================
 * 7. MENU ACTION IMPLEMENTATIONS (10 Core Features)
 * ============================================================================
 */

/**
 * Feature 1: Create an account with unique account number, customer name, 
 * address, phone number, account type, and opening balance.
 */
void ui_create_account(void) {
    printf("\n--- [CREATE NEW BANK ACCOUNT] ---\n");

    int acc_no = 0;
    while (true) {
        if (!safe_read_int(&acc_no, "Enter Unique Account Number (e.g. 1001): ")) {
            printf("[Error] Invalid input. Please enter a valid positive integer.\n");
            continue;
        }
        if (acc_no <= 0) {
            printf("[Error] Account number must be greater than zero.\n");
            continue;
        }
        if (hash_search(&g_hash_table, acc_no) != NULL) {
            printf("[Error] Account number %d already exists! Please choose another number.\n", acc_no);
            continue;
        }
        break;
    }

    char name[MAX_NAME_LEN];
    while (true) {
        if (!safe_read_line(name, sizeof(name), "Enter Customer Full Name: ") || strlen(name) == 0) {
            printf("[Error] Name cannot be empty.\n");
            continue;
        }
        break;
    }

    char address[MAX_ADDR_LEN];
    while (true) {
        if (!safe_read_line(address, sizeof(address), "Enter Residential / Branch Address: ") || strlen(address) == 0) {
            printf("[Error] Address cannot be empty.\n");
            continue;
        }
        break;
    }

    char phone[MAX_PHONE_LEN];
    while (true) {
        if (!safe_read_line(phone, sizeof(phone), "Enter Phone Number: ") || strlen(phone) < 7) {
            printf("[Error] Please enter a valid phone number (at least 7 characters).\n");
            continue;
        }
        break;
    }

    char acc_type[MAX_TYPE_LEN];
    while (true) {
        int type_choice = 0;
        printf("Select Account Type:\n  1. Savings Account\n  2. Current Account\nChoice (1 or 2): ");
        if (!safe_read_int(&type_choice, "")) {
            printf("[Error] Please enter 1 or 2.\n");
            continue;
        }
        if (type_choice == 1) {
            strcpy(acc_type, "Savings");
            break;
        } else if (type_choice == 2) {
            strcpy(acc_type, "Current");
            break;
        } else {
            printf("[Error] Invalid selection. Choose 1 for Savings or 2 for Current.\n");
        }
    }

    double opening_balance = 0.0;
    while (true) {
        if (!safe_read_double(&opening_balance, "Enter Opening Balance ($): ")) {
            printf("[Error] Invalid amount. Please enter a numeric value.\n");
            continue;
        }
        if (opening_balance < 0.0) {
            printf("[Error] Opening balance cannot be negative.\n");
            continue;
        }
        break;
    }

    Account* acc = create_account_record(acc_no, name, address, phone, acc_type, opening_balance);
    if (!acc) {
        printf("[Error] Failed to initialize account.\n");
        return;
    }

    if (!register_account_in_system(acc)) {
        printf("[Error] Failed to register account into ADS trees/hash table.\n");
        free(acc);
        return;
    }

    // Log Opening Balance in Transaction Linked List
    record_transaction(acc, "OPENING_BALANCE", opening_balance);

    printf("\n[Success] Bank Account #%d successfully created and indexed!\n", acc_no);
    printf("------------------------------------------------------------\n");
    printf("  Holder Name   : %s\n", acc->name);
    printf("  Address       : %s\n", acc->address);
    printf("  Phone Number  : %s\n", acc->phone);
    printf("  Account Type  : %s\n", acc->account_type);
    printf("  Initial Funds : $%.2f\n", acc->balance);
    printf("------------------------------------------------------------\n\n");
}

/**
 * Feature 2: Search for and display account details.
 */
void ui_search_account(void) {
    printf("\n--- [SEARCH ACCOUNT DETAILS] ---\n");

    int acc_no = 0;
    if (!safe_read_int(&acc_no, "Enter Account Number to Search: ")) {
        printf("[Error] Invalid account number format.\n");
        return;
    }

    // 1. Instant O(1) Lookup via Hash Table
    Account* acc = hash_search(&g_hash_table, acc_no);

    if (!acc) {
        printf("[Notice] Account #%d not found in the banking system.\n\n", acc_no);
        return;
    }

    printf("\n============================================================\n");
    printf("                 BANK ACCOUNT DETAILS : #%d\n", acc->account_number);
    printf("============================================================\n");
    printf("  Account Number    : %d\n", acc->account_number);
    printf("  Customer Name     : %s\n", acc->name);
    printf("  Residential Addr  : %s\n", acc->address);
    printf("  Phone Number      : %s\n", acc->phone);
    printf("  Account Type      : %s\n", acc->account_type);
    printf("  Available Balance : $%.2f\n", acc->balance);
    printf("  Logged Tx Count   : %d\n", acc->transaction_count);
    printf("  Total Loan Records: %d\n", acc->loan_count);
    printf("============================================================\n\n");

    if (acc->loan_count > 0) {
        display_account_loans(acc);
    }
}

/**
 * Feature 3: Deposit money.
 */
void ui_deposit(void) {
    printf("\n--- [DEPOSIT FUNDS] ---\n");

    int acc_no = 0;
    if (!safe_read_int(&acc_no, "Enter Account Number: ")) {
        printf("[Error] Invalid account number.\n");
        return;
    }

    Account* acc = hash_search(&g_hash_table, acc_no);
    if (!acc) {
        printf("[Error] Account #%d does not exist.\n\n", acc_no);
        return;
    }

    printf("Account Holder: %s | Current Balance: $%.2f\n", acc->name, acc->balance);

    double amount = 0.0;
    if (!safe_read_double(&amount, "Enter Deposit Amount ($): ")) {
        printf("[Error] Invalid numeric amount.\n");
        return;
    }

    if (amount <= 0.0) {
        printf("[Error] Deposit amount must be strictly greater than $0.00.\n\n");
        return;
    }

    acc->balance += amount;
    record_transaction(acc, "DEPOSIT", amount);

    printf("\n[Success] Deposit of $%.2f completed successfully!\n", amount);
    printf("Updated Balance for Account #%d: $%.2f\n\n", acc->account_number, acc->balance);
}

/**
 * Feature 4: Withdraw money, preventing overdrafts.
 */
void ui_withdraw(void) {
    printf("\n--- [WITHDRAW FUNDS] ---\n");

    int acc_no = 0;
    if (!safe_read_int(&acc_no, "Enter Account Number: ")) {
        printf("[Error] Invalid account number.\n");
        return;
    }

    Account* acc = hash_search(&g_hash_table, acc_no);
    if (!acc) {
        printf("[Error] Account #%d does not exist.\n\n", acc_no);
        return;
    }

    printf("Account Holder: %s | Current Balance: $%.2f\n", acc->name, acc->balance);

    double amount = 0.0;
    if (!safe_read_double(&amount, "Enter Withdrawal Amount ($): ")) {
        printf("[Error] Invalid numeric amount.\n");
        return;
    }

    if (amount <= 0.0) {
        printf("[Error] Withdrawal amount must be strictly greater than $0.00.\n\n");
        return;
    }

    // Overdraft Prevention Check
    if (amount > acc->balance) {
        printf("\n[REJECTED - INSUFFICIENT FUNDS]\n");
        printf("Requested Amount: $%.2f | Available Balance: $%.2f\n", amount, acc->balance);
        printf("Transaction aborted to prevent overdraft.\n\n");
        return;
    }

    acc->balance -= amount;
    record_transaction(acc, "WITHDRAWAL", amount);

    printf("\n[Success] Withdrawal of $%.2f completed successfully!\n", amount);
    printf("Updated Balance for Account #%d: $%.2f\n\n", acc->account_number, acc->balance);
}

/**
 * Feature 5: Transfer money between accounts.
 */
void ui_transfer(void) {
    printf("\n--- [FUND TRANSFER BETWEEN ACCOUNTS] ---\n");

    int src_acc_no = 0;
    if (!safe_read_int(&src_acc_no, "Enter Source (Sender) Account Number: ")) {
        printf("[Error] Invalid source account number.\n");
        return;
    }

    Account* src = hash_search(&g_hash_table, src_acc_no);
    if (!src) {
        printf("[Error] Sender Account #%d not found.\n\n", src_acc_no);
        return;
    }

    int dest_acc_no = 0;
    if (!safe_read_int(&dest_acc_no, "Enter Destination (Beneficiary) Account Number: ")) {
        printf("[Error] Invalid destination account number.\n");
        return;
    }

    if (src_acc_no == dest_acc_no) {
        printf("[Error] Source and Destination accounts cannot be identical.\n\n");
        return;
    }

    Account* dest = hash_search(&g_hash_table, dest_acc_no);
    if (!dest) {
        printf("[Error] Destination Account #%d not found.\n\n", dest_acc_no);
        return;
    }

    printf("Sender   : #%d (%s) | Balance: $%.2f\n", src->account_number, src->name, src->balance);
    printf("Recipient: #%d (%s)\n", dest->account_number, dest->name);

    double amount = 0.0;
    if (!safe_read_double(&amount, "Enter Transfer Amount ($): ")) {
        printf("[Error] Invalid transfer amount.\n");
        return;
    }

    if (amount <= 0.0) {
        printf("[Error] Transfer amount must be positive.\n\n");
        return;
    }

    if (amount > src->balance) {
        printf("\n[REJECTED - INSUFFICIENT FUNDS]\n");
        printf("Requested Transfer: $%.2f | Sender Balance: $%.2f\n", amount, src->balance);
        printf("Transaction cancelled.\n\n");
        return;
    }

    // Atomic execution
    src->balance -= amount;
    dest->balance += amount;

    char tx_src[32], tx_dest[32];
    snprintf(tx_src, sizeof(tx_src), "XFER_TO #%d", dest->account_number);
    snprintf(tx_dest, sizeof(tx_dest), "XFER_FROM #%d", src->account_number);

    record_transaction(src, tx_src, amount);
    record_transaction(dest, tx_dest, amount);

    printf("\n[Success] Transferred $%.2f from #%d (%s) to #%d (%s).\n",
           amount, src->account_number, src->name, dest->account_number, dest->name);
    printf("Sender Remaining Balance: $%.2f\n\n", src->balance);
}

/**
 * Feature 6: Display an account’s transaction history.
 */
void ui_transaction_history(void) {
    printf("\n--- [PASSBOOK / TRANSACTION STATEMENT] ---\n");

    int acc_no = 0;
    if (!safe_read_int(&acc_no, "Enter Account Number: ")) {
        printf("[Error] Invalid account number format.\n");
        return;
    }

    Account* acc = hash_search(&g_hash_table, acc_no);
    if (!acc) {
        printf("[Error] Account #%d does not exist.\n\n", acc_no);
        return;
    }

    display_transaction_history(acc);
}

/**
 * Feature 7: Apply for a loan, record loan amount, interest rate, term, and status.
 */
void ui_apply_loan(void) {
    printf("\n--- [APPLY FOR BANK LOAN] ---\n");

    int acc_no = 0;
    if (!safe_read_int(&acc_no, "Enter Borrower Account Number: ")) {
        printf("[Error] Invalid account number.\n");
        return;
    }

    Account* acc = hash_search(&g_hash_table, acc_no);
    if (!acc) {
        printf("[Error] Account #%d not found.\n\n", acc_no);
        return;
    }

    printf("Borrower Name: %s | Current Account Balance: $%.2f\n", acc->name, acc->balance);

    double principal = 0.0;
    if (!safe_read_double(&principal, "Enter Requested Loan Principal Amount ($): ")) {
        printf("[Error] Invalid principal amount.\n");
        return;
    }
    if (principal <= 0.0) {
        printf("[Error] Loan principal must be greater than $0.00.\n\n");
        return;
    }

    double interest_rate = 0.0;
    if (!safe_read_double(&interest_rate, "Enter Annual Interest Rate (%%, e.g. 7.5): ")) {
        printf("[Error] Invalid interest rate.\n");
        return;
    }
    if (interest_rate <= 0.0 || interest_rate > 100.0) {
        printf("[Error] Interest rate must be between 0.1%% and 100%%.\n\n");
        return;
    }

    int term_months = 0;
    if (!safe_read_int(&term_months, "Enter Loan Term in Months (e.g. 12, 24, 36): ")) {
        printf("[Error] Invalid loan term.\n");
        return;
    }
    if (term_months <= 0 || term_months > 360) {
        printf("[Error] Loan term must be between 1 and 360 months.\n\n");
        return;
    }

    // Sanction loan and add to account's loan linked list
    LoanNode* loan = add_loan_record(acc, principal, interest_rate, term_months);
    if (!loan) {
        printf("[Error] Failed to sanction loan.\n");
        return;
    }

    // Auto-disburse funds into customer account
    acc->balance += principal;
    record_transaction(acc, "LOAN_DISBURSED", principal);

    double monthly_emi = loan->total_due / (double)term_months;

    printf("\n============================================================\n");
    printf("                LOAN SANCTION LETTER / DETAILS              \n");
    printf("============================================================\n");
    printf("  Sanctioned Loan ID    : %d\n", loan->loan_id);
    printf("  Borrower Account      : #%d (%s)\n", acc->account_number, acc->name);
    printf("  Principal Disbursed   : $%.2f\n", loan->principal_amount);
    printf("  Annual Interest Rate  : %.2f%%\n", loan->interest_rate);
    printf("  Repayment Term        : %d Months\n", loan->term_months);
    printf("  Calculated Total Due  : $%.2f\n", loan->total_due);
    printf("  Approximate Monthly   : $%.2f / month\n", monthly_emi);
    printf("  Outstanding Balance   : $%.2f\n", loan->outstanding_balance);
    printf("  Loan Status           : %s\n", loan->status);
    printf("  Disbursement Date     : %s\n", loan->applied_date);
    printf("  New Account Balance   : $%.2f (Funds credited)\n", acc->balance);
    printf("============================================================\n\n");
}

/**
 * Feature 8: Repay a loan and update its outstanding balance.
 */
void ui_repay_loan(void) {
    printf("\n--- [REPAY LOAN] ---\n");

    int acc_no = 0;
    if (!safe_read_int(&acc_no, "Enter Account Number: ")) {
        printf("[Error] Invalid account number.\n");
        return;
    }

    Account* acc = hash_search(&g_hash_table, acc_no);
    if (!acc) {
        printf("[Error] Account #%d not found.\n\n", acc_no);
        return;
    }

    if (acc->loan_count == 0) {
        printf("[Info] Account #%d has no active or past loans.\n\n", acc_no);
        return;
    }

    display_account_loans(acc);

    int loan_id = 0;
    if (!safe_read_int(&loan_id, "Enter Loan ID to Repay: ")) {
        printf("[Error] Invalid Loan ID.\n");
        return;
    }

    LoanNode* loan = find_account_loan(acc, loan_id);
    if (!loan) {
        printf("[Error] Loan ID #%d not found under account #%d.\n\n", loan_id, acc_no);
        return;
    }

    if (strcmp(loan->status, "PAID_OFF") == 0 || loan->outstanding_balance <= 0.001) {
        printf("[Info] Loan #%d is already completely PAID OFF!\n\n", loan_id);
        return;
    }

    printf("Loan #%d | Outstanding Balance: $%.2f\n", loan->loan_id, loan->outstanding_balance);
    printf("Customer Account Balance: $%.2f\n", acc->balance);

    double payment = 0.0;
    if (!safe_read_double(&payment, "Enter Repayment Amount ($): ")) {
        printf("[Error] Invalid payment amount.\n");
        return;
    }

    if (payment <= 0.0) {
        printf("[Error] Payment must be strictly greater than $0.00.\n\n");
        return;
    }

    // Repayment payment source
    int source_choice = 0;
    printf("Choose Payment Method:\n");
    printf("  1. Deduct from Account Balance ($%.2f available)\n", acc->balance);
    printf("  2. Direct Cash / External Payment\nChoice (1 or 2): ");
    if (!safe_read_int(&source_choice, "") || (source_choice != 1 && source_choice != 2)) {
        printf("[Error] Invalid payment method chosen.\n");
        return;
    }

    if (source_choice == 1) {
        if (payment > acc->balance) {
            printf("[Error] Insufficient funds in account balance ($%.2f) to make this payment.\n\n", acc->balance);
            return;
        }
        acc->balance -= payment;
        char tx_type[32];
        snprintf(tx_type, sizeof(tx_type), "LOAN_REPAY #%d", loan->loan_id);
        record_transaction(acc, tx_type, payment);
    } else {
        char tx_type[32];
        snprintf(tx_type, sizeof(tx_type), "CASH_LOAN_PAY #%d", loan->loan_id);
        record_transaction(acc, tx_type, payment);
    }

    // Apply payment to outstanding loan balance
    if (payment >= loan->outstanding_balance) {
        double change = payment - loan->outstanding_balance;
        loan->outstanding_balance = 0.0;
        strncpy(loan->status, "PAID_OFF", sizeof(loan->status) - 1);
        printf("\n============================================================\n");
        printf(" [CONGRATULATIONS!] LOAN #%d HAS BEEN FULLY PAID OFF!\n", loan->loan_id);
        if (change > 0.01) {
            printf(" Overpayment refunded: $%.2f\n", change);
            if (source_choice == 1) {
                acc->balance += change; // Refund excess back to account
            }
        }
        printf("============================================================\n\n");
    } else {
        loan->outstanding_balance -= payment;
        printf("\n[Success] Repayment of $%.2f recorded for Loan #%d.\n", payment, loan->loan_id);
        printf("Remaining Outstanding Balance: $%.2f\n\n", loan->outstanding_balance);
    }
}

/**
 * Feature 9: List or sort accounts by account number and display account summaries.
 * Uses Inorder Traversal of the AVL Tree (Strictly O(N) ascending order).
 */
void ui_list_accounts_sorted(void) {
    printf("\n===========================================================================================\n");
    printf("                     ALL BANK ACCOUNTS (SORTED BY ACCOUNT NUMBER)                          \n");
    printf("===========================================================================================\n");
    printf("%-9s | %-20s | %-9s | %-13s | %-12s | %-12s | %-4s\n",
           "ACC NO", "CUSTOMER NAME", "TYPE", "PHONE", "BALANCE($)", "LOANS($)", "TXS");
    printf("-------------------------------------------------------------------------------------------\n");

    if (g_avl_root == NULL) {
        printf("                              No bank accounts currently on record.\n");
        printf("===========================================================================================\n\n");
        return;
    }

    int total_accounts = 0;
    double total_deposits = 0.0;
    double total_loans = 0.0;

    avl_inorder_traverse(g_avl_root, &total_accounts, &total_deposits, &total_loans);

    printf("===========================================================================================\n");
    printf("  Total Registered Accounts : %d\n", total_accounts);
    printf("  Total Bank Deposits       : $%.2f\n", total_deposits);
    if (total_accounts > 0) {
        printf("  Average Account Balance   : $%.2f\n", total_deposits / total_accounts);
    }
    printf("  Total Outstanding Loans   : $%.2f\n", total_loans);
    printf("===========================================================================================\n\n");
}

/**
 * Feature 10: Save records to file manually.
 */
void ui_save_database(void) {
    printf("\n--- [SAVE DATABASE TO FILE] ---\n");
    if (save_database(DB_FILENAME)) {
        printf("[Success] All accounts, transactions, and loans saved to '%s'.\n\n", DB_FILENAME);
    } else {
        printf("[Error] Failed to write database to disk.\n\n");
    }
}

/**
 * Loads records from file manually.
 */
void ui_load_database(void) {
    printf("\n--- [LOAD DATABASE FROM FILE] ---\n");
    printf("Warning: This will reload records from '%s'.\n", DB_FILENAME);
    
    // Clear existing data
    system_cleanup();
    hash_init(&g_hash_table);

    if (load_database(DB_FILENAME)) {
        printf("[Success] Reload complete.\n\n");
    } else {
        printf("[Info] Fresh system initialized.\n\n");
    }
}

/**
 * Additional ADS Feature: Close / Delete Account.
 * Demonstrates AVL Tree deletion O(log N) and Hash deletion O(1).
 */
void ui_delete_account(void) {
    printf("\n--- [CLOSE / DELETE BANK ACCOUNT] ---\n");

    int acc_no = 0;
    if (!safe_read_int(&acc_no, "Enter Account Number to Close: ")) {
        printf("[Error] Invalid account number format.\n");
        return;
    }

    Account* acc = hash_search(&g_hash_table, acc_no);
    if (!acc) {
        printf("[Error] Account #%d does not exist.\n\n", acc_no);
        return;
    }

    printf("Account Holder: %s | Balance: $%.2f\n", acc->name, acc->balance);
    if (acc->balance > 0.0) {
        printf("[Notice] Please withdraw remaining balance of $%.2f before closing.\n", acc->balance);
    }

    char confirm[16];
    if (!safe_read_line(confirm, sizeof(confirm), "Are you sure you want to PERMANENTLY delete this account? (yes/no): ")) {
        return;
    }

    if (strcmp(confirm, "yes") == 0 || strcmp(confirm, "y") == 0 || strcmp(confirm, "YES") == 0) {
        if (delete_account_from_system(acc_no)) {
            printf("[Success] Account #%d permanently removed from AVL Tree and Hash Table.\n\n", acc_no);
        }
    } else {
        printf("[Cancelled] Account closure cancelled.\n\n");
    }
}

/**
 * Additional ADS Feature: Visual AVL Tree Hierarchy.
 */
void ui_visualize_avl_tree(void) {
    printf("\n================================================================\n");
    printf("         AVL TREE HIERARCHICAL STRUCTURE (ASCII ROTATION VIEW)  \n");
    printf("  Node Format: [#AccountNumber | Height | BalanceFactor]        \n");
    printf("================================================================\n");
    if (!g_avl_root) {
        printf("Tree is empty.\n");
    } else {
        avl_print_visual(g_avl_root, 0);
    }
    printf("\n================================================================\n\n");
}

/**
 * Additional ADS Feature: Load Pre-configured Sample Dataset for Testing.
 */
void load_sample_dataset(void) {
    printf("\n[System] Populating demonstration accounts, transactions, and loans...\n");

    struct SampleData {
        int acc_no;
        const char* name;
        const char* address;
        const char* phone;
        const char* type;
        double balance;
    } samples[] = {
        {1001, "Alice Walker",   "742 Evergreen Terrace, Springfield", "555-0101", "Savings", 5500.00},
        {1002, "Bob Martin",     "221B Baker Street, London",          "555-0102", "Current", 12400.50},
        {1003, "Charlie Davis",  "42 Wallaby Way, Sydney",             "555-0103", "Savings", 3200.00},
        {1004, "Diana Prince",   "1007 Mountain Drive, Gotham",        "555-0104", "Current", 45000.00},
        {1005, "Edward Norton",  "350 Fifth Avenue, New York",         "555-0105", "Savings", 890.00},
        {1006, "Fiona Gallagher","2110 S Homan Ave, Chicago",         "555-0106", "Savings", 1750.25},
        {1007, "George Clark",   "10 Downing Street, London",          "555-0107", "Current", 28000.00}
    };

    int count = sizeof(samples) / sizeof(samples[0]);
    int inserted = 0;

    for (int i = 0; i < count; i++) {
        if (hash_search(&g_hash_table, samples[i].acc_no) == NULL) {
            Account* acc = create_account_record(samples[i].acc_no, samples[i].name, 
                                                 samples[i].address, samples[i].phone, 
                                                 samples[i].type, samples[i].balance);
            if (acc && register_account_in_system(acc)) {
                record_transaction(acc, "OPENING_BALANCE", samples[i].balance);
                inserted++;
            }
        }
    }

    // Add sample loans
    Account* alice = hash_search(&g_hash_table, 1001);
    if (alice && alice->loan_count == 0) {
        LoanNode* l = add_loan_record(alice, 5000.0, 6.5, 24);
        if (l) {
            alice->balance += 5000.0;
            record_transaction(alice, "LOAN_DISBURSED", 5000.0);
        }
    }

    Account* bob = hash_search(&g_hash_table, 1002);
    if (bob && bob->loan_count == 0) {
        LoanNode* l = add_loan_record(bob, 15000.0, 7.0, 36);
        if (l) {
            bob->balance += 15000.0;
            record_transaction(bob, "LOAN_DISBURSED", 15000.0);
        }
    }

    printf("[Success] %d demonstration accounts & sample loan facilities loaded!\n\n", inserted);
}

/* ============================================================================
 * 8. MAIN MENU & SYSTEM ENTRY POINT
 * ============================================================================
 */

void print_main_menu(void) {
    printf("========================================================================\n");
    printf("             BANK MANAGEMENT SYSTEM (ADS IN C)                          \n");
    printf("========================================================================\n");
    printf("  [ACCOUNT TRANSACTIONS - HASH TABLE & LINKED LIST]                     \n");
    printf("   1. Create New Account                  (AVL + Hash Table Insert)     \n");
    printf("   2. Search & Display Account Details    (Hash Table O(1) Fast Lookup) \n");
    printf("   3. Deposit Money                       (Hash O(1) + Tx Linked List)  \n");
    printf("   4. Withdraw Money (Overdraft Protected)(Hash O(1) + Tx Linked List)  \n");
    printf("   5. Transfer Money Between Accounts     (Atomic Transaction Logging)  \n");
    printf("   6. Display Account Transaction History (Passbook Linked List)        \n");
    printf("  ----------------------------------------------------------------------\n");
    printf("  [LOAN MANAGEMENT SUBSYSTEM]                                           \n");
    printf("   7. Apply for a Loan                    (Interest & Repayment Engine) \n");
    printf("   8. Repay a Loan                        (Balance & Status Update)     \n");
    printf("  ----------------------------------------------------------------------\n");
    printf("  [ADS REPORTS, STRUCTURES & PERSISTENCE]                               \n");
    printf("   9. List & Sort Accounts by Number      (AVL Tree Inorder Traversal)  \n");
    printf("  10. Save Records to File                (File Serialization)          \n");
    printf("  11. Reload Records from File            (File Deserialization)        \n");
    printf("  12. Close / Delete an Account           (AVL Tree O(log N) + Hash Del)\n");
    printf("  13. Visual AVL Tree Structure           (Height & Balance Factors)    \n");
    printf("  14. Hash Table Diagnostics              (Buckets, Collisions, Metrics)\n");
    printf("  15. Load Demo / Sample Dataset          (Instant Testing Suite)       \n");
    printf("   0. Exit System                         (Auto-Save & Memory Cleanup)  \n");
    printf("========================================================================\n");
    printf("Enter your choice (0-15): ");
}

int main(void) {
    // 1. Initialize data structures
    hash_init(&g_hash_table);
    g_avl_root = NULL;

    printf("\n========================================================================\n");
    printf("   WELCOME TO THE ADVANCED DATA STRUCTURES BANK MANAGEMENT SYSTEM       \n");
    printf("========================================================================\n");

    // 2. Automatically load records if file exists
    load_database(DB_FILENAME);

    // 3. Main interactive loop
    int choice = -1;
    while (choice != 0) {
        print_main_menu();
        if (!safe_read_int(&choice, "")) {
            printf("[Error] Please enter a valid number from the menu.\n\n");
            continue;
        }

        switch (choice) {
            case 1:
                ui_create_account();
                break;
            case 2:
                ui_search_account();
                break;
            case 3:
                ui_deposit();
                break;
            case 4:
                ui_withdraw();
                break;
            case 5:
                ui_transfer();
                break;
            case 6:
                ui_transaction_history();
                break;
            case 7:
                ui_apply_loan();
                break;
            case 8:
                ui_repay_loan();
                break;
            case 9:
                ui_list_accounts_sorted();
                break;
            case 10:
                ui_save_database();
                break;
            case 11:
                ui_load_database();
                break;
            case 12:
                ui_delete_account();
                break;
            case 13:
                ui_visualize_avl_tree();
                break;
            case 14:
                hash_display_metrics(&g_hash_table);
                break;
            case 15:
                load_sample_dataset();
                break;
            case 0:
                printf("\nSaving database records to '%s'...\n", DB_FILENAME);
                save_database(DB_FILENAME);
                printf("Cleaning up memory and deallocating dynamic data structures...\n");
                system_cleanup();
                printf("System terminated successfully. Goodbye!\n\n");
                break;
            default:
                printf("[Error] Option %d is not recognized. Please choose from 0 to 15.\n\n", choice);
                break;
        }
    }

    return 0;
}
