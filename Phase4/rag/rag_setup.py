"""
rag_setup.py — RAG Index Builder

Walks docs/, chunks markdown (~500 tokens, 50 overlap),
embeds with sentence-transformers/all-MiniLM-L6-v2,
and persists to vector_store/ using ChromaDB.

Idempotent: rebuilds the index on every run (delete + recreate).

Usage:
    python rag/rag_setup.py
    python rag/rag_setup.py --docs-dir docs --persist-dir vector_store
"""

import os
import sys
import argparse
import glob


def chunk_text(text: str, chunk_size: int = 500, overlap: int = 50) -> list:
    """Split text into overlapping chunks by approximate token count.

    Uses whitespace-based splitting as a token approximation.
    Each chunk is approximately `chunk_size` tokens with `overlap` token
    overlap between consecutive chunks.

    Args:
        text: The full document text to chunk.
        chunk_size: Target number of tokens per chunk.
        overlap: Number of overlapping tokens between chunks.

    Returns:
        List of text chunk strings.
    """
    words = text.split()
    if len(words) <= chunk_size:
        return [text.strip()] if text.strip() else []

    chunks = []
    start = 0
    while start < len(words):
        end = start + chunk_size
        chunk = " ".join(words[start:end])
        if chunk.strip():
            chunks.append(chunk.strip())
        start += chunk_size - overlap

    return chunks


def load_documents(docs_dir: str) -> list:
    """Load all markdown files from the docs directory.

    Args:
        docs_dir: Path to directory containing markdown files.

    Returns:
        List of dicts with keys: 'text', 'source', 'chunk_index'.
    """
    documents = []
    md_files = glob.glob(os.path.join(docs_dir, "**", "*.md"), recursive=True)

    if not md_files:
        print(f"Warning: No markdown files found in {docs_dir}/")
        return documents

    for filepath in sorted(md_files):
        filename = os.path.basename(filepath)
        # Skip .gitkeep and other non-doc files
        if filename.startswith("."):
            continue

        with open(filepath, "r", encoding="utf-8") as f:
            content = f.read()

        if not content.strip():
            continue

        chunks = chunk_text(content)
        for i, chunk in enumerate(chunks):
            documents.append({
                "text": chunk,
                "source": filename,
                "chunk_index": i,
            })

    return documents


def build_index(docs_dir: str = "docs", persist_dir: str = "vector_store") -> None:
    """Build or rebuild the ChromaDB vector index from docs/.

    Args:
        docs_dir: Path to directory containing markdown source documents.
        persist_dir: Path to ChromaDB persistence directory.
    """
    try:
        import chromadb
        from chromadb.utils import embedding_functions
    except ImportError:
        print("Error: chromadb is not installed.")
        print("Run: pip install chromadb sentence-transformers")
        sys.exit(1)

    # Resolve paths relative to Phase4/ root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    docs_path = os.path.join(project_root, docs_dir)
    persist_path = os.path.join(project_root, persist_dir)

    print(f"RAG Setup — Building index")
    print(f"  Docs directory:    {docs_path}")
    print(f"  Persist directory: {persist_path}")

    # Load and chunk documents
    documents = load_documents(docs_path)
    if not documents:
        print("Error: No documents found to index.")
        sys.exit(1)

    print(f"  Documents loaded:  {len(documents)} chunks from "
          f"{len(set(d['source'] for d in documents))} files")

    # Create embedding function using HuggingFace model
    print("  Loading embedding model: all-MiniLM-L6-v2 ...")
    ef = embedding_functions.SentenceTransformerEmbeddingFunction(
        model_name="all-MiniLM-L6-v2"
    )

    # Initialize ChromaDB persistent client
    os.makedirs(persist_path, exist_ok=True)
    client = chromadb.PersistentClient(path=persist_path)

    # Delete existing collection if it exists (idempotent rebuild)
    collection_name = "phase4_docs"
    existing = [c.name for c in client.list_collections()]
    if collection_name in existing:
        print(f"  Deleting existing collection '{collection_name}'...")
        client.delete_collection(collection_name)

    # Create collection with embedding function
    collection = client.create_collection(
        name=collection_name,
        embedding_function=ef,
        metadata={"description": "Phase 4 documentation for RAG retrieval"}
    )

    # Add documents to collection
    ids = []
    texts = []
    metadatas = []

    for i, doc in enumerate(documents):
        ids.append(f"doc_{i}")
        texts.append(doc["text"])
        metadatas.append({
            "source": doc["source"],
            "chunk_index": doc["chunk_index"],
        })

    collection.add(
        ids=ids,
        documents=texts,
        metadatas=metadatas,
    )

    print(f"  Indexed {len(ids)} chunks into collection '{collection_name}'")
    print(f"  Index persisted to: {persist_path}")
    print("RAG Setup — Complete")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Build RAG index from docs/")
    parser.add_argument("--docs-dir", default="docs",
                        help="Path to docs directory (default: docs)")
    parser.add_argument("--persist-dir", default="vector_store",
                        help="Path to vector store (default: vector_store)")
    args = parser.parse_args()
    build_index(args.docs_dir, args.persist_dir)
