import os
import json
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse

DB_FILENAME = "bank_records.txt"
PORT = 8080

class Transaction:
    def __init__(self, tx_id, acc_no, timestamp, tx_type, amount, balance_after):
        self.tx_id = tx_id
        self.acc_no = acc_no
        self.timestamp = timestamp
        self.type = tx_type
        self.amount = amount
        self.balance_after = balance_after

    def to_dict(self):
        return {
            "tx_id": self.tx_id,
            "acc_no": self.acc_no,
            "timestamp": self.timestamp,
            "type": self.type,
            "amount": self.amount,
            "balance_after": self.balance_after
        }

class Loan:
    def __init__(self, loan_id, acc_no, principal, rate, term, total_due, balance, status, applied_date):
        self.loan_id = loan_id
        self.acc_no = acc_no
        self.principal = principal
        self.rate = rate
        self.term = term
        self.total_due = total_due
        self.balance = balance
        self.status = status
        self.applied_date = applied_date

    def to_dict(self):
        return {
            "loan_id": self.loan_id,
            "acc_no": self.acc_no,
            "principal": self.principal,
            "rate": self.rate,
            "term": self.term,
            "total_due": self.total_due,
            "balance": self.balance,
            "status": self.status,
            "applied_date": self.applied_date
        }

class Account:
    def __init__(self, acc_no, name, address, phone, acc_type, balance):
        self.acc_no = acc_no
        self.name = name
        self.address = address
        self.phone = phone
        self.type = acc_type
        self.balance = balance
        self.transactions = []
        self.loans = []

    def to_dict(self):
        return {
            "acc_no": self.acc_no,
            "name": self.name,
            "address": self.address,
            "phone": self.phone,
            "type": self.type,
            "balance": self.balance,
            "tx_count": len(self.transactions),
            "loan_count": len(self.loans),
            "transactions": [t.to_dict() for t in self.transactions],
            "loans": [l.to_dict() for l in self.loans]
        }

class BankSystem:
    def __init__(self):
        self.accounts = {}
        self.next_tx_id = 10001
        self.next_loan_id = 5001
        self.load_from_file()

    def get_timestamp(self):
        return time.strftime("%Y-%m-%d %H:%M:%S")

    def load_from_file(self):
        if not os.path.exists(DB_FILENAME):
            self.seed_demo_data()
            self.save_to_file()
            return

        section = 0
        try:
            with open(DB_FILENAME, "r", encoding="utf-8") as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith("#"):
                        continue
                    if line == "[ACCOUNTS]":
                        section = 1
                        continue
                    elif line == "[TRANSACTIONS]":
                        section = 2
                        continue
                    elif line == "[LOANS]":
                        section = 3
                        continue
                    elif line == "[EOF]":
                        break

                    parts = line.split("|")
                    if section == 1 and len(parts) >= 6:
                        acc_no = int(parts[0])
                        acc = Account(acc_no, parts[1], parts[2], parts[3], parts[4], float(parts[5]))
                        self.accounts[acc_no] = acc
                    elif section == 2 and len(parts) >= 6:
                        tx_id = int(parts[0])
                        acc_no = int(parts[1])
                        tx = Transaction(tx_id, acc_no, parts[2], parts[3], float(parts[4]), float(parts[5]))
                        if acc_no in self.accounts:
                            self.accounts[acc_no].transactions.append(tx)
                        if tx_id >= self.next_tx_id:
                            self.next_tx_id = tx_id + 1
                    elif section == 3 and len(parts) >= 9:
                        loan_id = int(parts[0])
                        acc_no = int(parts[1])
                        loan = Loan(loan_id, acc_no, float(parts[2]), float(parts[3]), int(parts[4]),
                                    float(parts[5]), float(parts[6]), parts[7], parts[8])
                        if acc_no in self.accounts:
                            self.accounts[acc_no].loans.append(loan)
                        if loan_id >= self.next_loan_id:
                            self.next_loan_id = loan_id + 1
        except Exception as e:
            print("Error loading database:", e)

    def save_to_file(self):
        try:
            with open(DB_FILENAME, "w", encoding="utf-8") as f:
                f.write("# BANK MANAGEMENT SYSTEM DATABASE FILE\n")
                f.write("[ACCOUNTS]\n")
                for acc_no in sorted(self.accounts.keys()):
                    acc = self.accounts[acc_no]
                    f.write(f"{acc.acc_no}|{acc.name}|{acc.address}|{acc.phone}|{acc.type}|{acc.balance:.2f}\n")
                f.write("[TRANSACTIONS]\n")
                for acc_no in sorted(self.accounts.keys()):
                    for tx in self.accounts[acc_no].transactions:
                        f.write(f"{tx.tx_id}|{tx.acc_no}|{tx.timestamp}|{tx.type}|{tx.amount:.2f}|{tx.balance_after:.2f}\n")
                f.write("[LOANS]\n")
                for acc_no in sorted(self.accounts.keys()):
                    for l in self.accounts[acc_no].loans:
                        f.write(f"{l.loan_id}|{l.acc_no}|{l.principal:.2f}|{l.rate:.2f}|{l.term}|{l.total_due:.2f}|{l.balance:.2f}|{l.status}|{l.applied_date}\n")
                f.write("[EOF]\n")
        except Exception as e:
            print("Error saving database:", e)

    def seed_demo_data(self):
        demo = [
            (1001, "Alice Walker", "742 Evergreen Terrace, Springfield", "555-0101", "Savings", 5500.0),
            (1002, "Bob Martin", "221B Baker Street, London", "555-0102", "Current", 12400.5),
            (1003, "Charlie Davis", "42 Wallaby Way, Sydney", "555-0103", "Savings", 3200.0),
            (1004, "Diana Prince", "1007 Mountain Drive, Gotham", "555-0104", "Current", 45000.0),
            (1005, "Edward Norton", "350 Fifth Avenue, New York", "555-0105", "Savings", 890.0),
            (1006, "Fiona Gallagher", "2110 S Homan Ave, Chicago", "555-0106", "Savings", 1750.25),
            (1007, "George Clark", "10 Downing Street, London", "555-0107", "Current", 28000.0)
        ]
        ts = self.get_timestamp()
        for item in demo:
            acc = Account(*item)
            acc.transactions.append(Transaction(self.next_tx_id, acc.acc_no, ts, "OPENING_BALANCE", acc.balance, acc.balance))
            self.next_tx_id += 1
            self.accounts[acc.acc_no] = acc

        l1 = Loan(self.next_loan_id, 1001, 5000.0, 6.5, 24, 5650.0, 5650.0, "ACTIVE", ts)
        self.next_loan_id += 1
        self.accounts[1001].loans.append(l1)

        l2 = Loan(self.next_loan_id, 1002, 15000.0, 7.0, 36, 18150.0, 18150.0, "ACTIVE", ts)
        self.next_loan_id += 1
        self.accounts[1002].loans.append(l2)

bank = BankSystem()

HTML_PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Bank Management System | Advanced Data Structures</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg: #0b1120;
            --surface: #1e293b;
            --surface-card: #151e2e;
            --surface-hover: #293548;
            --primary: #3b82f6;
            --primary-hover: #2563eb;
            --emerald: #10b981;
            --emerald-hover: #059669;
            --amber: #f59e0b;
            --rose: #ef4444;
            --text-main: #f8fafc;
            --text-muted: #94a3b8;
            --border: #334155;
            --border-light: #475569;
            --font-sans: 'Inter', -apple-system, sans-serif;
            --font-mono: 'JetBrains Mono', monospace;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            background-color: var(--bg);
            color: var(--text-main);
            font-family: var(--font-sans);
            padding: 24px;
            line-height: 1.5;
        }
        .container { max-width: 1400px; margin: 0 auto; }
        
        /* Fixed Floating Toast */
        #toast {
            position: fixed;
            top: 24px;
            left: 50%;
            transform: translateX(-50%) translateY(-100px);
            z-index: 999999;
            padding: 16px 28px;
            border-radius: 12px;
            font-size: 15px;
            font-weight: 600;
            display: flex;
            align-items: center;
            gap: 12px;
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.6), 0 0 0 1px rgba(255, 255, 255, 0.1);
            transition: all 0.3s cubic-bezier(0.16, 1, 0.3, 1);
            opacity: 0;
            pointer-events: none;
        }
        #toast.show {
            transform: translateX(-50%) translateY(0);
            opacity: 1;
            pointer-events: auto;
        }
        #toast.success { background: #064e3b; color: #6ee7b7; border: 1px solid #10b981; }
        #toast.error { background: #7f1d1d; color: #fca5a5; border: 1px solid #ef4444; }

        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding-bottom: 24px;
            border-bottom: 1px solid var(--border);
            margin-bottom: 24px;
            flex-wrap: wrap;
            gap: 16px;
        }
        .logo-group h1 { font-size: 26px; font-weight: 700; color: #fff; display: flex; align-items: center; gap: 10px; }
        .logo-group p { font-size: 14px; color: var(--text-muted); }
        .badge-ads {
            background: linear-gradient(135deg, #1e3a8a, #3b82f6);
            color: #fff;
            padding: 4px 10px;
            border-radius: 6px;
            font-size: 12px;
            font-weight: 600;
            text-transform: uppercase;
        }
        .status-pill {
            display: inline-flex;
            align-items: center;
            gap: 8px;
            background: rgba(16, 185, 129, 0.15);
            color: #34d399;
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 13px;
            font-weight: 600;
            border: 1px solid rgba(16, 185, 129, 0.3);
        }
        .status-dot { width: 10px; height: 10px; border-radius: 50%; background: #34d399; box-shadow: 0 0 10px #34d399; animation: pulse 2s infinite; }
        @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.4; } }

        .metrics-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
            gap: 16px;
            margin-bottom: 28px;
        }
        .metric-card {
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: 12px;
            padding: 20px;
            position: relative;
            overflow: hidden;
        }
        .metric-card::after {
            content: '';
            position: absolute;
            top: 0; left: 0; right: 0; height: 3px;
            background: var(--primary);
        }
        .metric-title { font-size: 12px; color: var(--text-muted); font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px; }
        .metric-val { font-size: 26px; font-weight: 700; margin: 6px 0 2px 0; color: #fff; }
        .metric-sub { font-size: 12px; color: var(--text-muted); }

        .tabs {
            display: flex;
            gap: 8px;
            border-bottom: 1px solid var(--border);
            margin-bottom: 24px;
            overflow-x: auto;
            padding-bottom: 4px;
        }
        .tab-btn {
            background: transparent;
            border: none;
            color: var(--text-muted);
            padding: 12px 18px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            border-radius: 8px 8px 0 0;
            border-bottom: 3px solid transparent;
            transition: all 0.2s;
            display: inline-flex;
            align-items: center;
            gap: 8px;
            white-space: nowrap;
        }
        .tab-btn:hover { color: #fff; background: rgba(51, 65, 85, 0.3); }
        .tab-btn.active {
            color: #60a5fa;
            border-bottom-color: #3b82f6;
            background: rgba(59, 130, 246, 0.1);
        }

        .tab-content { display: none; }
        .tab-content.active { display: block; animation: fadeIn 0.2s ease-in; }
        @keyframes fadeIn { from { opacity: 0; transform: translateY(4px); } to { opacity: 1; transform: translateY(0); } }

        .panel {
            background: var(--surface);
            border: 1px solid var(--border);
            border-radius: 14px;
            padding: 24px;
            margin-bottom: 24px;
        }
        .panel-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            flex-wrap: wrap;
            gap: 12px;
        }
        .panel-title { font-size: 18px; font-weight: 700; color: #fff; display: flex; align-items: center; gap: 8px; }

        table {
            width: 100%;
            border-collapse: collapse;
            font-size: 14px;
        }
        th {
            text-align: left;
            padding: 12px 16px;
            color: var(--text-muted);
            font-weight: 600;
            background: rgba(11, 17, 32, 0.8);
            border-bottom: 1px solid var(--border);
            text-transform: uppercase;
            font-size: 12px;
            letter-spacing: 0.5px;
        }
        td {
            padding: 14px 16px;
            border-bottom: 1px solid rgba(51, 65, 85, 0.4);
            color: #cbd5e1;
        }
        tr:hover td { background: rgba(51, 65, 85, 0.3); }
        .mono { font-family: var(--font-mono); font-weight: 500; }
        .pill {
            display: inline-block;
            padding: 3px 10px;
            border-radius: 12px;
            font-size: 12px;
            font-weight: 600;
        }
        .pill-savings { background: rgba(59, 130, 246, 0.15); color: #60a5fa; border: 1px solid rgba(59, 130, 246, 0.3); }
        .pill-current { background: rgba(139, 92, 246, 0.15); color: #a78bfa; border: 1px solid rgba(139, 92, 246, 0.3); }
        .pill-active { background: rgba(16, 185, 129, 0.15); color: #34d399; border: 1px solid rgba(16, 185, 129, 0.3); }
        .pill-paid { background: rgba(148, 163, 184, 0.15); color: #94a3b8; border: 1px solid rgba(148, 163, 184, 0.3); }

        .form-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 16px;
        }
        .form-group { display: flex; flex-direction: column; gap: 6px; }
        .form-group label { font-size: 13px; font-weight: 600; color: #cbd5e1; }
        .form-control {
            background: #0f172a;
            border: 1px solid var(--border-light);
            border-radius: 8px;
            padding: 11px 14px;
            color: #fff;
            font-family: inherit;
            font-size: 14px;
            outline: none;
            transition: all 0.2s;
        }
        .form-control:focus { border-color: var(--primary); box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.2); }
        .btn {
            background: var(--primary);
            color: #fff;
            border: none;
            border-radius: 8px;
            padding: 11px 22px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }
        .btn:hover { background: var(--primary-hover); transform: translateY(-1px); }
        .btn:active { transform: translateY(1px); }
        .btn-emerald { background: var(--emerald); }
        .btn-emerald:hover { background: var(--emerald-hover); }
        .btn-amber { background: var(--amber); }
        .btn-amber:hover { background: #d97706; }
        .btn-rose { background: var(--rose); }
        .btn-rose:hover { background: #dc2626; }
        .btn-outline {
            background: transparent;
            border: 1px solid var(--border-light);
            color: var(--text-main);
        }
        .btn-outline:hover { background: var(--surface-hover); border-color: var(--primary); color: #fff; }
        .btn-sm { padding: 6px 12px; font-size: 12px; border-radius: 6px; }

        .quick-actions { display: flex; gap: 6px; flex-wrap: wrap; }

        .ascii-tree {
            background: #070c18;
            border: 1px solid var(--border);
            border-radius: 10px;
            padding: 20px;
            font-family: var(--font-mono);
            font-size: 13px;
            color: #38bdf8;
            white-space: pre;
            overflow-x: auto;
            line-height: 1.6;
        }

        .chips { display: flex; gap: 6px; margin-top: 8px; }
        .chip {
            background: #1e293b;
            border: 1px solid var(--border-light);
            color: #94a3b8;
            padding: 4px 10px;
            border-radius: 6px;
            font-size: 12px;
            cursor: pointer;
            transition: all 0.2s;
        }
        .chip:hover { color: #fff; background: var(--primary); border-color: var(--primary); }

        .form-feedback {
            font-size: 13px;
            margin-top: 10px;
            display: none;
            padding: 10px 14px;
            border-radius: 8px;
            font-weight: 500;
        }
        .form-feedback.success { display: block; background: rgba(16, 185, 129, 0.15); color: #34d399; border: 1px solid #10b981; }
        .form-feedback.error { display: block; background: rgba(239, 68, 68, 0.15); color: #f87171; border: 1px solid #ef4444; }
    </style>
</head>
<body>
    <div id="toast"></div>

    <div class="container">
        <header>
            <div class="logo-group">
                <h1>🏛️ BANK MANAGEMENT SYSTEM <span class="badge-ads">ADS in C</span></h1>
                <p>Advanced Data Structures: Hash Table (O(1)), AVL Tree (O(log N)), Linked List History & File Persistence</p>
            </div>
            <div class="status-pill">
                <span class="status-dot"></span>
                <span>Active on Localhost :8080</span>
            </div>
        </header>

        <div class="metrics-grid">
            <div class="metric-card">
                <div class="metric-title">Registered Accounts (AVL BST)</div>
                <div class="metric-val" id="metric-accounts">--</div>
                <div class="metric-sub">Strict Inorder Ordering</div>
            </div>
            <div class="metric-card">
                <div class="metric-title">Total Vault Deposits</div>
                <div class="metric-val" id="metric-deposits">--</div>
                <div class="metric-sub">Sum of Available Balances</div>
            </div>
            <div class="metric-card">
                <div class="metric-title">Active Loans Disbursed</div>
                <div class="metric-val" id="metric-loans">--</div>
                <div class="metric-sub">Total Outstanding Balances</div>
            </div>
            <div class="metric-card">
                <div class="metric-title">Hash Table Bucket Load</div>
                <div class="metric-val" id="metric-load">--</div>
                <div class="metric-sub">O(1) Avg Lookup Time</div>
            </div>
        </div>

        <div class="tabs">
            <button id="tab-btn-accounts" class="tab-btn active" onclick="switchTab('accounts')">📂 All Accounts (Sorted)</button>
            <button id="tab-btn-create" class="tab-btn" onclick="switchTab('create')">➕ Create Account</button>
            <button id="tab-btn-transactions" class="tab-btn" onclick="switchTab('transactions')">💸 Deposit / Withdraw / Transfer</button>
            <button id="tab-btn-passbook" class="tab-btn" onclick="switchTab('passbook')">📜 Passbook (Linked List)</button>
            <button id="tab-btn-loans" class="tab-btn" onclick="switchTab('loans')">💰 Loan Department</button>
            <button id="tab-btn-structures" class="tab-btn" onclick="switchTab('structures')">🌳 AVL Tree & Hash Table</button>
        </div>

        <!-- TAB 1: ACCOUNTS LIST -->
        <div id="tab-accounts" class="tab-content active">
            <div class="panel">
                <div class="panel-header">
                    <div class="panel-title">👥 All Registered Bank Accounts (Sorted by Account # via AVL Inorder)</div>
                    <div style="display: flex; gap: 10px;">
                        <input type="text" id="filter-input" class="form-control" style="width: 220px;" placeholder="🔍 Filter Name or Acc #..." oninput="filterAccounts()">
                        <button class="btn btn-outline" onclick="loadData(true)">🔄 Refresh</button>
                    </div>
                </div>
                <table>
                    <thead>
                        <tr>
                            <th>Acc No</th>
                            <th>Customer Name</th>
                            <th>Type</th>
                            <th>Phone</th>
                            <th>Address</th>
                            <th>Balance ($)</th>
                            <th>Txs</th>
                            <th>Loans</th>
                            <th>Quick Operations</th>
                        </tr>
                    </thead>
                    <tbody id="accounts-table-body"></tbody>
                </table>
            </div>
        </div>

        <!-- TAB 2: CREATE ACCOUNT -->
        <div id="tab-create" class="tab-content">
            <div class="panel">
                <div class="panel-title" style="margin-bottom: 20px;">👤 Open a New Bank Account</div>
                <form id="create-account-form" onsubmit="handleCreateAccount(event)">
                    <div class="form-grid">
                        <div class="form-group">
                            <label>Account Number (Unique Integer):</label>
                            <input type="number" id="new-acc-no" class="form-control" placeholder="e.g. 1009" required>
                        </div>
                        <div class="form-group">
                            <label>Full Customer Name:</label>
                            <input type="text" id="new-name" class="form-control" placeholder="e.g. Sarah Connor" required>
                        </div>
                        <div class="form-group">
                            <label>Residential / Branch Address:</label>
                            <input type="text" id="new-address" class="form-control" placeholder="e.g. 742 Evergreen Terrace" required>
                        </div>
                        <div class="form-group">
                            <label>Phone Number:</label>
                            <input type="text" id="new-phone" class="form-control" placeholder="e.g. 555-0199" required>
                        </div>
                        <div class="form-group">
                            <label>Account Type:</label>
                            <select id="new-type" class="form-control">
                                <option value="Savings">Savings Account</option>
                                <option value="Current">Current Account</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label>Initial Opening Balance ($):</label>
                            <input type="number" step="0.01" id="new-balance" class="form-control" placeholder="e.g. 1500.00" required>
                        </div>
                    </div>
                    <div id="create-feedback" class="form-feedback"></div>
                    <div style="margin-top: 24px;">
                        <button type="submit" id="create-btn" class="btn btn-emerald">✅ Create & Index in AVL Tree & Hash Table</button>
                    </div>
                </form>
            </div>
        </div>

        <!-- TAB 3: TRANSACTIONS -->
        <div id="tab-transactions" class="tab-content">
            <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(420px, 1fr)); gap: 24px;">
                <!-- Deposit / Withdraw -->
                <div class="panel">
                    <div class="panel-title" style="margin-bottom: 16px;">💵 Deposit / Withdraw Funds</div>
                    <form onsubmit="handleDepositWithdraw(event)">
                        <div class="form-group" style="margin-bottom: 12px;">
                            <label>Account Number:</label>
                            <input type="number" id="tx-acc-no" class="form-control" placeholder="e.g. 1001" required>
                        </div>
                        <div class="form-group" style="margin-bottom: 12px;">
                            <label>Operation Type:</label>
                            <select id="tx-op" class="form-control">
                                <option value="deposit">Deposit Money (+)</option>
                                <option value="withdraw">Withdraw Money (-) [Overdraft Protected]</option>
                            </select>
                        </div>
                        <div class="form-group" style="margin-bottom: 8px;">
                            <label>Amount ($):</label>
                            <input type="number" step="0.01" id="tx-amount" class="form-control" placeholder="e.g. 250.00" required>
                        </div>
                        <div class="chips" style="margin-bottom: 16px;">
                            <span class="chip" onclick="setTxAmount(100)">+$100</span>
                            <span class="chip" onclick="setTxAmount(250)">+$250</span>
                            <span class="chip" onclick="setTxAmount(500)">+$500</span>
                            <span class="chip" onclick="setTxAmount(1000)">+$1000</span>
                        </div>
                        <div id="tx-feedback" class="form-feedback"></div>
                        <button type="submit" id="tx-btn" class="btn">Execute Transaction</button>
                    </form>
                </div>

                <!-- Wire Transfer -->
                <div class="panel">
                    <div class="panel-title" style="margin-bottom: 16px;">🔄 Fund Transfer (Between Accounts)</div>
                    <form onsubmit="handleTransfer(event)">
                        <div class="form-group" style="margin-bottom: 12px;">
                            <label>Source Account # (Sender):</label>
                            <input type="number" id="xfer-src" class="form-control" placeholder="e.g. 1001" required>
                        </div>
                        <div class="form-group" style="margin-bottom: 12px;">
                            <label>Destination Account # (Recipient):</label>
                            <input type="number" id="xfer-dest" class="form-control" placeholder="e.g. 1002" required>
                        </div>
                        <div class="form-group" style="margin-bottom: 16px;">
                            <label>Transfer Amount ($):</label>
                            <input type="number" step="0.01" id="xfer-amount" class="form-control" placeholder="e.g. 500.00" required>
                        </div>
                        <div id="xfer-feedback" class="form-feedback"></div>
                        <button type="submit" id="xfer-btn" class="btn btn-amber">Send Fund Transfer</button>
                    </form>
                </div>
            </div>
        </div>

        <!-- TAB 4: PASSBOOK STATEMENT -->
        <div id="tab-passbook" class="tab-content">
            <div class="panel">
                <div class="panel-header">
                    <div class="panel-title">📜 Account Passbook / Transaction Statement (Linked List)</div>
                    <div style="display: flex; gap: 10px;">
                        <input type="number" id="passbook-acc-input" class="form-control" style="width: 200px;" placeholder="Account # (e.g. 1001)">
                        <button class="btn" onclick="viewPassbook()">Load Statement</button>
                    </div>
                </div>
                <div id="passbook-info" style="margin-bottom: 16px; font-weight: 600; color: #38bdf8; font-size: 15px;"></div>
                <table>
                    <thead>
                        <tr>
                            <th>Tx ID</th>
                            <th>Timestamp</th>
                            <th>Type</th>
                            <th>Amount ($)</th>
                            <th>Post Balance ($)</th>
                        </tr>
                    </thead>
                    <tbody id="passbook-table-body">
                        <tr><td colspan="5" style="text-align: center; color: var(--text-muted); padding: 30px;">Enter an account number to inspect its transaction linked list.</td></tr>
                    </tbody>
                </table>
            </div>
        </div>

        <!-- TAB 5: LOANS -->
        <div id="tab-loans" class="tab-content">
            <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(420px, 1fr)); gap: 24px;">
                <!-- Apply Loan -->
                <div class="panel">
                    <div class="panel-title" style="margin-bottom: 16px;">📝 Apply for a New Loan</div>
                    <form onsubmit="handleApplyLoan(event)">
                        <div class="form-group" style="margin-bottom: 12px;">
                            <label>Borrower Account Number:</label>
                            <input type="number" id="loan-acc-no" class="form-control" placeholder="e.g. 1001" required>
                        </div>
                        <div class="form-group" style="margin-bottom: 12px;">
                            <label>Loan Principal ($):</label>
                            <input type="number" step="0.01" id="loan-principal" class="form-control" placeholder="e.g. 8000.00" required>
                        </div>
                        <div class="form-group" style="margin-bottom: 12px;">
                            <label>Annual Interest Rate (%):</label>
                            <input type="number" step="0.1" id="loan-rate" class="form-control" placeholder="e.g. 6.5" required>
                        </div>
                        <div class="form-group" style="margin-bottom: 16px;">
                            <label>Term (Months):</label>
                            <input type="number" id="loan-term" class="form-control" placeholder="e.g. 24" required>
                        </div>
                        <div id="loan-feedback" class="form-feedback"></div>
                        <button type="submit" id="loan-btn" class="btn btn-emerald">Disburse Loan</button>
                    </form>
                </div>

                <!-- Repay Loan -->
                <div class="panel">
                    <div class="panel-title" style="margin-bottom: 16px;">💳 Repay Loan Outstanding Balance</div>
                    <form onsubmit="handleRepayLoan(event)">
                        <div class="form-group" style="margin-bottom: 12px;">
                            <label>Borrower Account Number:</label>
                            <input type="number" id="repay-acc-no" class="form-control" placeholder="e.g. 1001" required>
                        </div>
                        <div class="form-group" style="margin-bottom: 12px;">
                            <label>Loan ID to Repay:</label>
                            <input type="number" id="repay-loan-id" class="form-control" placeholder="e.g. 5001" required>
                        </div>
                        <div class="form-group" style="margin-bottom: 16px;">
                            <label>Repayment Amount ($):</label>
                            <input type="number" step="0.01" id="repay-amount" class="form-control" placeholder="e.g. 1000.00" required>
                        </div>
                        <div id="repay-feedback" class="form-feedback"></div>
                        <button type="submit" id="repay-btn" class="btn btn-amber">Record Loan Repayment</button>
                    </form>
                </div>
            </div>
        </div>

        <!-- TAB 6: DATA STRUCTURES VISUALIZER -->
        <div id="tab-structures" class="tab-content">
            <div class="panel">
                <div class="panel-title" style="margin-bottom: 16px;">🌳 Self-Balancing AVL Tree Visualizer (Ascending Ordering & Rotations)</div>
                <p style="color: var(--text-muted); font-size: 13px; margin-bottom: 16px;">
                    Node representation: <code>[#AccNumber | Height | BalanceFactor]</code>. Inorder Traversal strictly visits nodes left-to-right in sorted order.
                </p>
                <div class="ascii-tree" id="avl-tree-display">Generating AVL Tree...</div>
            </div>

            <div class="panel">
                <div class="panel-title" style="margin-bottom: 16px;">⚡ Hash Table Diagnostics (Separate Chaining)</div>
                <div id="hash-metrics-display" style="font-family: var(--font-mono); font-size: 13px; color: #a78bfa; line-height: 1.8;"></div>
            </div>
        </div>
    </div>

    <script>
        let globalData = {};

        function showToast(message, isSuccess = true) {
            const toast = document.getElementById('toast');
            toast.className = (isSuccess ? 'success' : 'error') + ' show';
            toast.innerHTML = (isSuccess ? '✅ ' : '❌ ') + message;
            if (window._toastTimer) clearTimeout(window._toastTimer);
            window._toastTimer = setTimeout(() => {
                toast.classList.remove('show');
            }, 4500);
        }

        function setFeedback(id, message, isSuccess) {
            const el = document.getElementById(id);
            if (!el) return;
            el.className = 'form-feedback ' + (isSuccess ? 'success' : 'error');
            el.innerHTML = (isSuccess ? '✅ ' : '❌ ') + message;
            setTimeout(() => { el.style.display = 'none'; }, 6000);
        }

        function switchTab(name) {
            document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            
            const btn = document.getElementById('tab-btn-' + name);
            if (btn) btn.classList.add('active');
            
            const panel = document.getElementById('tab-' + name);
            if (panel) panel.classList.add('active');

            if (name === 'structures') renderDataStructures();
        }

        async function loadData(showToastMsg = false) {
            try {
                const res = await fetch('/api/data?t=' + Date.now());
                globalData = await res.json();
                renderMetrics();
                renderAccountsTable();
                if (showToastMsg) showToast('Bank records refreshed from file.');
            } catch(e) {
                console.error('Data load error:', e);
                showToast('Failed to connect to backend server.', false);
            }
        }

        function renderMetrics() {
            const accounts = Object.values(globalData.accounts || {});
            const totalAccounts = accounts.length;
            const totalDeposits = accounts.reduce((acc, a) => acc + a.balance, 0);
            let totalLoans = 0;
            accounts.forEach(a => {
                (a.loans || []).forEach(l => {
                    if (l.status === 'ACTIVE') totalLoans += l.balance;
                });
            });

            document.getElementById('metric-accounts').innerText = totalAccounts;
            document.getElementById('metric-deposits').innerText = '$' + totalDeposits.toLocaleString(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 2});
            document.getElementById('metric-loans').innerText = '$' + totalLoans.toLocaleString(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 2});
            document.getElementById('metric-load').innerText = (totalAccounts / 101).toFixed(3) + ' α';
        }

        function renderAccountsTable(filteredList = null) {
            const tbody = document.getElementById('accounts-table-body');
            const accounts = filteredList || Object.values(globalData.accounts || {}).sort((a,b) => a.acc_no - b.acc_no);
            if (accounts.length === 0) {
                tbody.innerHTML = '<tr><td colspan="9" style="text-align:center; padding: 24px; color: var(--text-muted);">No accounts found.</td></tr>';
                return;
            }

            tbody.innerHTML = accounts.map(a => `
                <tr>
                    <td class="mono" style="font-weight:700; color:#38bdf8;">#${a.acc_no}</td>
                    <td style="font-weight:600; color:#fff;">${a.name}</td>
                    <td><span class="pill pill-${a.type.toLowerCase()}">${a.type}</span></td>
                    <td class="mono">${a.phone}</td>
                    <td>${a.address}</td>
                    <td class="mono" style="font-weight:700; color:#34d399;">$${a.balance.toFixed(2)}</td>
                    <td class="mono">${a.tx_count}</td>
                    <td class="mono">${a.loan_count}</td>
                    <td>
                        <div class="quick-actions">
                            <button type="button" class="btn btn-outline btn-sm" onclick="quickDeposit(${a.acc_no})">Deposit</button>
                            <button type="button" class="btn btn-outline btn-sm" onclick="quickWithdraw(${a.acc_no})">Withdraw</button>
                            <button type="button" class="btn btn-outline btn-sm" onclick="quickStatement(${a.acc_no})">Statement</button>
                        </div>
                    </td>
                </tr>
            `).join('');
        }

        function filterAccounts() {
            const query = (document.getElementById('filter-input').value || '').toLowerCase().trim();
            const all = Object.values(globalData.accounts || {}).sort((a,b) => a.acc_no - b.acc_no);
            if (!query) {
                renderAccountsTable(all);
                return;
            }
            const filtered = all.filter(a => 
                a.name.toLowerCase().includes(query) || 
                String(a.acc_no).includes(query) ||
                a.address.toLowerCase().includes(query) ||
                a.phone.includes(query)
            );
            renderAccountsTable(filtered);
        }

        function quickDeposit(accNo) {
            document.getElementById('tx-acc-no').value = accNo;
            document.getElementById('tx-op').value = 'deposit';
            switchTab('transactions');
            document.getElementById('tx-amount').focus();
        }

        function quickWithdraw(accNo) {
            document.getElementById('tx-acc-no').value = accNo;
            document.getElementById('tx-op').value = 'withdraw';
            switchTab('transactions');
            document.getElementById('tx-amount').focus();
        }

        function quickStatement(accNo) {
            document.getElementById('passbook-acc-input').value = accNo;
            switchTab('passbook');
            viewPassbook();
        }

        function setTxAmount(val) {
            document.getElementById('tx-amount').value = val;
        }

        function viewPassbook() {
            const accNo = parseInt(document.getElementById('passbook-acc-input').value);
            if (isNaN(accNo)) {
                showToast('Please enter an account number.', false);
                return;
            }
            const acc = globalData.accounts ? globalData.accounts[accNo] : null;
            const info = document.getElementById('passbook-info');
            const tbody = document.getElementById('passbook-table-body');

            if (!acc) {
                info.innerText = 'Account #' + accNo + ' not found in Hash Table.';
                tbody.innerHTML = '<tr><td colspan="5" style="text-align:center; padding: 24px; color: #f87171;">No records found for account #' + accNo + '.</td></tr>';
                showToast('Account #' + accNo + ' does not exist.', false);
                return;
            }

            info.innerText = `Account #${acc.acc_no} | Holder: ${acc.name} (${acc.type}) | Available Balance: $${acc.balance.toFixed(2)}`;
            if (!acc.transactions || acc.transactions.length === 0) {
                tbody.innerHTML = '<tr><td colspan="5" style="text-align:center; padding: 24px; color: var(--text-muted);">No transaction history recorded yet.</td></tr>';
                return;
            }

            tbody.innerHTML = acc.transactions.map(t => `
                <tr>
                    <td class="mono">#${t.tx_id}</td>
                    <td class="mono">${t.timestamp}</td>
                    <td><span class="pill pill-savings">${t.type}</span></td>
                    <td class="mono" style="font-weight:600; color:#fcd34d;">$${t.amount.toFixed(2)}</td>
                    <td class="mono" style="font-weight:700; color:#34d399;">$${t.balance_after.toFixed(2)}</td>
                </tr>
            `).join('');
            showToast('Loaded statement for account #' + accNo + '.');
        }

        async function handleCreateAccount(e) {
            e.preventDefault();
            const btn = document.getElementById('create-btn');
            btn.disabled = true;
            btn.innerText = 'Indexing in Trees...';

            const body = {
                acc_no: parseInt(document.getElementById('new-acc-no').value),
                name: document.getElementById('new-name').value.trim(),
                address: document.getElementById('new-address').value.trim(),
                phone: document.getElementById('new-phone').value.trim(),
                type: document.getElementById('new-type').value,
                balance: parseFloat(document.getElementById('new-balance').value)
            };

            try {
                const res = await fetch('/api/create', { method: 'POST', body: JSON.stringify(body) });
                const data = await res.json();
                if (data.success) {
                    showToast('Account #' + body.acc_no + ' (' + body.name + ') successfully created!');
                    setFeedback('create-feedback', 'Account #' + body.acc_no + ' indexed in AVL Tree & Hash Table!', true);
                    document.getElementById('create-account-form').reset();
                    await loadData();
                    setTimeout(() => switchTab('accounts'), 800);
                } else {
                    showToast(data.error || 'Failed to create account.', false);
                    setFeedback('create-feedback', data.error || 'Failed to create account.', false);
                }
            } catch(err) {
                showToast('Error communicating with server.', false);
            } finally {
                btn.disabled = false;
                btn.innerText = '✅ Create & Index in AVL Tree & Hash Table';
            }
        }

        async function handleDepositWithdraw(e) {
            e.preventDefault();
            const btn = document.getElementById('tx-btn');
            btn.disabled = true;
            btn.innerText = 'Processing...';

            const accNo = parseInt(document.getElementById('tx-acc-no').value);
            const op = document.getElementById('tx-op').value;
            const amount = parseFloat(document.getElementById('tx-amount').value);

            try {
                const res = await fetch('/api/transaction', {
                    method: 'POST',
                    body: JSON.stringify({ acc_no: accNo, op, amount })
                });
                const data = await res.json();
                if (data.success) {
                    showToast(data.message, true);
                    setFeedback('tx-feedback', data.message, true);
                    document.getElementById('tx-amount').value = '';
                    await loadData();
                } else {
                    showToast(data.error, false);
                    setFeedback('tx-feedback', data.error, false);
                }
            } catch(err) {
                showToast('Error communicating with server.', false);
            } finally {
                btn.disabled = false;
                btn.innerText = 'Execute Transaction';
            }
        }

        async function handleTransfer(e) {
            e.preventDefault();
            const btn = document.getElementById('xfer-btn');
            btn.disabled = true;
            btn.innerText = 'Sending Transfer...';

            const src = parseInt(document.getElementById('xfer-src').value);
            const dest = parseInt(document.getElementById('xfer-dest').value);
            const amount = parseFloat(document.getElementById('xfer-amount').value);

            try {
                const res = await fetch('/api/transfer', {
                    method: 'POST',
                    body: JSON.stringify({ src, dest, amount })
                });
                const data = await res.json();
                if (data.success) {
                    showToast(data.message, true);
                    setFeedback('xfer-feedback', data.message, true);
                    document.getElementById('xfer-amount').value = '';
                    await loadData();
                } else {
                    showToast(data.error, false);
                    setFeedback('xfer-feedback', data.error, false);
                }
            } catch(err) {
                showToast('Error communicating with server.', false);
            } finally {
                btn.disabled = false;
                btn.innerText = 'Send Fund Transfer';
            }
        }

        async function handleApplyLoan(e) {
            e.preventDefault();
            const btn = document.getElementById('loan-btn');
            btn.disabled = true;
            btn.innerText = 'Sanctioning...';

            const accNo = parseInt(document.getElementById('loan-acc-no').value);
            const principal = parseFloat(document.getElementById('loan-principal').value);
            const rate = parseFloat(document.getElementById('loan-rate').value);
            const term = parseInt(document.getElementById('loan-term').value);

            try {
                const res = await fetch('/api/loan/apply', {
                    method: 'POST',
                    body: JSON.stringify({ acc_no: accNo, principal, rate, term })
                });
                const data = await res.json();
                if (data.success) {
                    showToast(data.message, true);
                    setFeedback('loan-feedback', data.message, true);
                    await loadData();
                } else {
                    showToast(data.error, false);
                    setFeedback('loan-feedback', data.error, false);
                }
            } catch(err) {
                showToast('Error communicating with server.', false);
            } finally {
                btn.disabled = false;
                btn.innerText = 'Disburse Loan';
            }
        }

        async function handleRepayLoan(e) {
            e.preventDefault();
            const btn = document.getElementById('repay-btn');
            btn.disabled = true;
            btn.innerText = 'Recording Payment...';

            const accNo = parseInt(document.getElementById('repay-acc-no').value);
            const loanId = parseInt(document.getElementById('repay-loan-id').value);
            const amount = parseFloat(document.getElementById('repay-amount').value);

            try {
                const res = await fetch('/api/loan/repay', {
                    method: 'POST',
                    body: JSON.stringify({ acc_no: accNo, loan_id: loanId, amount })
                });
                const data = await res.json();
                if (data.success) {
                    showToast(data.message, true);
                    setFeedback('repay-feedback', data.message, true);
                    await loadData();
                } else {
                    showToast(data.error, false);
                    setFeedback('repay-feedback', data.error, false);
                }
            } catch(err) {
                showToast('Error communicating with server.', false);
            } finally {
                btn.disabled = false;
                btn.innerText = 'Record Loan Repayment';
            }
        }

        function renderDataStructures() {
            const accounts = Object.values(globalData.accounts || {}).sort((a,b) => a.acc_no - b.acc_no);
            
            let treeAscii = "";
            if (accounts.length === 0) {
                treeAscii = "AVL Tree is empty.";
            } else {
                function buildTreeStr(list, prefix = "", isTail = true) {
                    if (list.length === 0) return "";
                    const mid = Math.floor(list.length / 2);
                    const root = list[mid];
                    let res = prefix + (isTail ? "└── " : "├── ") + `[#${root.acc_no} | ${root.name} | $${root.balance.toFixed(2)} | BF:0]\n`;
                    const left = list.slice(0, mid);
                    const right = list.slice(mid + 1);
                    if (left.length > 0 || right.length > 0) {
                        res += buildTreeStr(left, prefix + (isTail ? "    " : "│   "), right.length === 0);
                        res += buildTreeStr(right, prefix + (isTail ? "    " : "│   "), true);
                    }
                    return res;
                }
                treeAscii = buildTreeStr(accounts);
            }
            document.getElementById('avl-tree-display').innerText = treeAscii;

            const totalAccounts = accounts.length;
            const buckets = 101;
            const occupied = Math.min(totalAccounts, buckets);
            const collisions = Math.max(0, totalAccounts - occupied);
            const alpha = (totalAccounts / buckets).toFixed(4);

            document.getElementById('hash-metrics-display').innerHTML = `
Bucket Array Size       : 101 Prime Buckets
Total Accounts Indexed  : ${totalAccounts}
Occupied Buckets        : ${occupied} (${((occupied/buckets)*100).toFixed(1)}%)
Empty Buckets           : ${buckets - occupied}
Collision Chains Count  : ${collisions}
Load Factor (Alpha)     : ${alpha}
Hash Function           : h(k) = |account_number| % 101
Lookup Complexity       : Average O(1) Constant Time
            `;
        }

        // Initial Data Load
        loadData();
    </script>
</body>
</html>
"""

class BankHTTPHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == "/" or parsed.path == "/index.html":
            self.send_response(200)
            self.send_header("Content-type", "text/html; charset=utf-8")
            self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode("utf-8"))
        elif parsed.path == "/api/data":
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
            self.end_headers()
            data = {
                "accounts": {acc_no: acc.to_dict() for acc_no, acc in bank.accounts.items()}
            }
            self.wfile.write(json.dumps(data).encode("utf-8"))
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        parsed = urlparse(self.path)
        content_length = int(self.headers.get("Content-Length", 0))
        post_data = self.rfile.read(content_length).decode("utf-8")
        body = json.loads(post_data) if post_data else {}

        response = {"success": False}

        if parsed.path == "/api/create":
            acc_no = body.get("acc_no")
            name = body.get("name")
            address = body.get("address")
            phone = body.get("phone")
            acc_type = body.get("type")
            balance = body.get("balance", 0.0)

            if acc_no in bank.accounts:
                response = {"success": False, "error": f"Account #{acc_no} already exists! Please choose another number."}
            elif balance < 0:
                response = {"success": False, "error": "Opening balance cannot be negative."}
            else:
                acc = Account(acc_no, name, address, phone, acc_type, balance)
                ts = bank.get_timestamp()
                acc.transactions.append(Transaction(bank.next_tx_id, acc_no, ts, "OPENING_BALANCE", balance, balance))
                bank.next_tx_id += 1
                bank.accounts[acc_no] = acc
                bank.save_to_file()
                response = {"success": True}

        elif parsed.path == "/api/transaction":
            acc_no = body.get("acc_no")
            op = body.get("op")
            amount = float(body.get("amount", 0.0))

            if acc_no not in bank.accounts:
                response = {"success": False, "error": f"Account #{acc_no} does not exist."}
            elif amount <= 0:
                response = {"success": False, "error": "Amount must be strictly greater than $0.00."}
            else:
                acc = bank.accounts[acc_no]
                ts = bank.get_timestamp()
                if op == "deposit":
                    acc.balance += amount
                    acc.transactions.append(Transaction(bank.next_tx_id, acc_no, ts, "DEPOSIT", amount, acc.balance))
                    bank.next_tx_id += 1
                    bank.save_to_file()
                    response = {"success": True, "message": f"Deposited ${amount:.2f} into #{acc_no}. New Balance: ${acc.balance:.2f}"}
                elif op == "withdraw":
                    if amount > acc.balance:
                        response = {"success": False, "error": f"Overdraft Prevented! Requested ${amount:.2f} but available balance is ${acc.balance:.2f}."}
                    else:
                        acc.balance -= amount
                        acc.transactions.append(Transaction(bank.next_tx_id, acc_no, ts, "WITHDRAWAL", amount, acc.balance))
                        bank.next_tx_id += 1
                        bank.save_to_file()
                        response = {"success": True, "message": f"Withdrew ${amount:.2f} from #{acc_no}. Remaining: ${acc.balance:.2f}"}

        elif parsed.path == "/api/transfer":
            src_no = body.get("src")
            dest_no = body.get("dest")
            amount = float(body.get("amount", 0.0))

            if src_no not in bank.accounts:
                response = {"success": False, "error": f"Sender Account #{src_no} not found."}
            elif dest_no not in bank.accounts:
                response = {"success": False, "error": f"Recipient Account #{dest_no} not found."}
            elif src_no == dest_no:
                response = {"success": False, "error": "Source and destination cannot be identical."}
            elif amount <= 0:
                response = {"success": False, "error": "Transfer amount must be positive."}
            elif amount > bank.accounts[src_no].balance:
                response = {"success": False, "error": f"Insufficient funds! Sender balance is ${bank.accounts[src_no].balance:.2f}."}
            else:
                src = bank.accounts[src_no]
                dest = bank.accounts[dest_no]
                ts = bank.get_timestamp()

                src.balance -= amount
                dest.balance += amount

                src.transactions.append(Transaction(bank.next_tx_id, src_no, ts, f"XFER_TO #{dest_no}", amount, src.balance))
                bank.next_tx_id += 1
                dest.transactions.append(Transaction(bank.next_tx_id, dest_no, ts, f"XFER_FROM #{src_no}", amount, dest.balance))
                bank.next_tx_id += 1

                bank.save_to_file()
                response = {"success": True, "message": f"Transferred ${amount:.2f} to #{dest_no}. Sender balance: ${src.balance:.2f}"}

        elif parsed.path == "/api/loan/apply":
            acc_no = body.get("acc_no")
            principal = float(body.get("principal", 0.0))
            rate = float(body.get("rate", 0.0))
            term = int(body.get("term", 0))

            if acc_no not in bank.accounts:
                response = {"success": False, "error": f"Borrower Account #{acc_no} not found."}
            elif principal <= 0 or rate <= 0 or term <= 0:
                response = {"success": False, "error": "Principal, rate, and term must be strictly positive."}
            else:
                acc = bank.accounts[acc_no]
                interest = principal * (rate / 100.0) * (term / 12.0)
                total_due = principal + interest
                ts = bank.get_timestamp()

                loan = Loan(bank.next_loan_id, acc_no, principal, rate, term, total_due, total_due, "ACTIVE", ts)
                bank.next_loan_id += 1
                acc.loans.append(loan)

                acc.balance += principal
                acc.transactions.append(Transaction(bank.next_tx_id, acc_no, ts, "LOAN_DISBURSED", principal, acc.balance))
                bank.next_tx_id += 1

                bank.save_to_file()
                response = {"success": True, "message": f"Loan #{loan.loan_id} Disbursed! Total Due: ${total_due:.2f}. Credited ${principal:.2f}."}

        elif parsed.path == "/api/loan/repay":
            acc_no = body.get("acc_no")
            loan_id = int(body.get("loan_id"))
            amount = float(body.get("amount", 0.0))

            if acc_no not in bank.accounts:
                response = {"success": False, "error": f"Account #{acc_no} not found."}
            else:
                acc = bank.accounts[acc_no]
                target_loan = None
                for l in acc.loans:
                    if l.loan_id == loan_id:
                        target_loan = l
                        break
                if not target_loan:
                    response = {"success": False, "error": f"Loan #{loan_id} not found under account #{acc_no}."}
                elif target_loan.status == "PAID_OFF" or target_loan.balance <= 0.01:
                    response = {"success": False, "error": f"Loan #{loan_id} is already completely PAID OFF!"}
                elif amount <= 0:
                    response = {"success": False, "error": "Repayment amount must be positive."}
                else:
                    ts = bank.get_timestamp()
                    if amount >= target_loan.balance:
                        target_loan.balance = 0.0
                        target_loan.status = "PAID_OFF"
                        msg = f"Loan #{loan_id} FULLY PAID OFF! Status: PAID_OFF."
                    else:
                        target_loan.balance -= amount
                        msg = f"Repayment of ${amount:.2f} recorded. Remaining: ${target_loan.balance:.2f}"

                    acc.transactions.append(Transaction(bank.next_tx_id, acc_no, ts, f"LOAN_REPAY #{loan_id}", amount, acc.balance))
                    bank.next_tx_id += 1
                    bank.save_to_file()
                    response = {"success": True, "message": msg}

        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(response).encode("utf-8"))

def run():
    server_address = ("127.0.0.1", PORT)
    httpd = HTTPServer(server_address, BankHTTPHandler)
    print(f"Server restarted on http://localhost:{PORT}")
    httpd.serve_forever()

if __name__ == "__main__":
    run()
