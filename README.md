# 🎵 Music BST Sequencer

This project implements a fully functional **Binary Search Tree (BST)** in C to store and manipulate musical notes. The BST organizes notes based on their position in a song and allows traversal, playback, and transformations like reversing and harmonizing.

---

## 🚀 Features

- Insert notes into a BST using a unique key system
- Search and delete notes
- Tree traversals:
  - In-order
  - Pre-order
  - Post-order
- Generate a playable playlist using in-order traversal
- Reverse a song (play it backwards)
- Harmonize notes by shifting pitch and timing

---

## 🧠 Key Concepts

- Binary Search Trees (BST)
- Recursion
- Tree traversal algorithms
- Dynamic memory management (malloc/free)
- Structs and pointers in C

---

## 🎼 How It Works

Each musical note is stored as a node with:
- `frequency` (pitch)
- `bar` (position in song)
- `index` (timing within the bar)

The BST key is computed as:
