# Bank Management System in C (Advanced Data Structures Project)

A complete, menu-driven **Bank Management System** implemented in pure standard C (ANSI C / C99 / C11) for an Advanced Data Structures (ADS) course. It uses **Hash Tables**, **AVL Trees (Self-Balancing BSTs)**, and **Linked Lists / Queues** to demonstrate fundamental computer science concepts applied to banking operations without relying on third-party database software.

---

## 🏛️ Advanced Data Structures Architecture

| Data Structure | Banking Functionality | Time Complexity (Search) | Time Complexity (Insert) | Time Complexity (Delete) | Space Complexity |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **Hash Table (Separate Chaining)** | Instant Account Lookup & Validation | $\mathcal{O}(1)$ avg | $\mathcal{O}(1)$ avg | $\mathcal{O}(1)$ avg | $\mathcal{O}(M + N)$ |
| **AVL Tree (Self-Balancing BST)** | Numerical Indexing & Sorted Account Summaries | $\mathcal{O}(\log N)$ | $\mathcal{O}(\log N)$ | $\mathcal{O}(\log N)$ | $\mathcal{O}(N)$ |
| **Singly Linked List / Queue** | Transaction History (Passbook Statement) | $\mathcal{O}(T)$ | $\mathcal{O}(1)$ | $\mathcal{O}(T)$ | $\mathcal{O}(T)$ |
| **Linked List** | Loan Portfolio & Outstanding Balance Tracking | $\mathcal{O}(L)$ | $\mathcal{O}(1)$ | $\mathcal{O}(L)$ | $\mathcal{O}(L)$ |

*Notation:*
* $N$ = Total registered bank accounts
* $M$ = Hash table bucket count (`HASH_TABLE_SIZE = 101`)
* $T$ = Number of transactions logged for an account
* $L$ = Number of loans attached to an account

---

## 🚀 Implemented Features

1. **Create Account**:
   - Collects unique Account Number, Customer Name, Address, Phone Number, Account Type (`Savings` or `Current`), and Opening Balance.
   - Rejects duplicate account numbers and invalid negative amounts.
   - Dual-indexes into both Hash Table ($O(1)$) and AVL Tree ($O(\log N)$).

2. **Search Account Details**:
   - Performs instant $O(1)$ lookup via the Hash Table.
   - Displays complete account profile, balance, transaction counts, and loan summary.

3. **Deposit Funds**:
   - Validates positive deposit amount.
   - Automatically credits balance and appends a timestamped `DEPOSIT` node to the transaction linked list.

4. **Withdraw Funds (Overdraft Prevention)**:
   - Validates positive amount and checks sufficient funds (`balance >= amount`).
   - Aborts if withdrawal would cause an overdraft.
   - Updates balance and appends a `WITHDRAWAL` transaction node.

5. **Transfer Funds Between Accounts**:
   - Validates that source and destination accounts exist and are distinct.
   - Checks sender's balance and atomically transfers funds.
   - Logs `XFER_TO` on sender statement and `XFER_FROM` on recipient statement.

6. **Display Transaction History (Passbook Statement)**:
   - Traverses the account's transaction linked list in chronological order.
   - Outputs Transaction ID, Timestamp, Type, Amount, and Post-Transaction Balance.

7. **Apply for a Loan**:
   - Records Loan Principal, Annual Interest Rate (%), and Term in Months.
   - Calculates Total Repayable Due, Simple Interest, and Monthly Installment (EMI).
   - Automatically credits loan principal to account balance and marks status as `ACTIVE`.

8. **Repay a Loan**:
   - Lists active loans for the account.
   - Allows payment via Account Balance deduction or Direct Cash payment.
   - Automatically caps repayment, updates outstanding balance, and transitions status to `PAID_OFF` when cleared.

9. **List & Sort Accounts by Account Number**:
   - Performs an **In-Order Traversal** ($Left \to Root \to Right$) on the AVL Tree in $O(N)$ time.
   - Automatically produces an ascending sorted list with account summaries, bank total deposits, and outstanding loan totals.

10. **File Persistence (Save & Load Without Database)**:
    - Automatically loads records on startup from `bank_records.txt`.
    - Automatically serializes records to `bank_records.txt` on exit.
    - Handles missing files safely without crashing.

11. **Additional ADS Utilities**:
    - **Visual ASCII AVL Tree Viewer**: Visualizes tree rotations, heights, and balance factors ($BF \in \{-1, 0, 1\}$).
    - **Hash Table Metrics**: Displays bucket occupancy, collision count, longest chain length, and load factor ($\alpha$).
    - **Close / Delete Account**: Rebalances AVL Tree with rotations and frees all associated dynamic memory.
    - **Sample Dataset Loader**: Pre-loads 7 sample accounts, transactions, and loans for instant testing.

---

## 🛠️ Compilation & Execution

The source code is contained entirely within `bank_management_system.c` and uses only standard C libraries (`stdio.h`, `stdlib.h`, `string.h`, `time.h`, `ctype.h`, `stdbool.h`).

### 1. Using GCC (MinGW / Linux / macOS)
```bash
gcc -Wall -Wextra -std=c99 bank_management_system.c -o bank_management_system
./bank_management_system
```

### 2. Using Clang
```bash
clang -Wall -Wextra -std=c99 bank_management_system.c -o bank_management_system
./bank_management_system
```

### 3. Using Microsoft Visual C++ (MSVC / Developer Command Prompt)
```cmd
cl.exe /W4 bank_management_system.c
bank_management_system.exe
```

### 4. Using IDEs (Dev-C++, Code::Blocks, Visual Studio Code)
1. Open `bank_management_system.c` in the IDE.
2. Select **Build & Run** (or press `F9` / `F11`).

---

## 📂 File Format Specification (`bank_records.txt`)

The file uses human-readable pipe-delimited (`|`) text formatted in distinct sections:

```text
# BANK MANAGEMENT SYSTEM DATABASE FILE
[ACCOUNTS]
1001|Alice Walker|742 Evergreen Terrace, Springfield|555-0101|Savings|10500.00
1002|Bob Martin|221B Baker Street, London|555-0102|Current|27400.50
[TRANSACTIONS]
10001|1001|2026-11-12 17:55:00|OPENING_BALANCE|5500.00|5500.00
10002|1001|2026-11-12 17:55:00|LOAN_DISBURSED|5000.00|10500.00
[LOANS]
5001|1001|5000.00|6.50|24|5650.00|5650.00|ACTIVE|2026-11-12 17:55:00
[EOF]
```
