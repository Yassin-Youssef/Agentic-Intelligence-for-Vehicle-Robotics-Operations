"""
rag_query.py — RAG Query Interface

Queries the ChromaDB vector store for relevant document chunks.
Used by the Search Agent (Sonnet) to retrieve context before
the Explanation Agent runs.

Usage:
    from rag.rag_query import query_rag
    results = query_rag("What is the temperature threshold?")
    for chunk in results:
        print(chunk)
"""

import os
import sys


def query_rag(question: str, top_k: int = 3,
              persist_dir: str = "vector_store") -> list:
    """Query the RAG index for relevant document chunks.

    Args:
        question: Natural language query string.
        top_k: Number of top results to return.
        persist_dir: Path to ChromaDB persistence directory.

    Returns:
        List of relevant text chunks from the document store.
        Returns empty list if the index doesn't exist or query fails.
    """
    try:
        import chromadb
        from chromadb.utils import embedding_functions
    except ImportError:
        print("Warning: chromadb not installed. RAG query skipped.")
        return []

    # Resolve path relative to Phase4/ root
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    persist_path = os.path.join(project_root, persist_dir)

    # Check if vector store exists
    if not os.path.isdir(persist_path):
        print(f"Warning: Vector store not found at {persist_path}. "
              f"Run 'python rag/rag_setup.py' first.")
        return []

    # Initialize ChromaDB client
    client = chromadb.PersistentClient(path=persist_path)

    # Check if collection exists
    collection_name = "phase4_docs"
    existing = [c.name for c in client.list_collections()]
    if collection_name not in existing:
        print(f"Warning: Collection '{collection_name}' not found. "
              f"Run 'python rag/rag_setup.py' first.")
        return []

    # Load embedding function (must match the one used during indexing)
    ef = embedding_functions.SentenceTransformerEmbeddingFunction(
        model_name="all-MiniLM-L6-v2"
    )

    # Get collection
    collection = client.get_collection(
        name=collection_name,
        embedding_function=ef,
    )

    # Query
    results = collection.query(
        query_texts=[question],
        n_results=min(top_k, collection.count()),
    )

    # Extract document texts
    chunks = []
    if results and results.get("documents"):
        for doc_list in results["documents"]:
            for doc in doc_list:
                chunks.append(doc)

    return chunks


def query_rag_with_metadata(question: str, top_k: int = 3,
                            persist_dir: str = "vector_store") -> list:
    """Query RAG and return chunks with source metadata.

    Args:
        question: Natural language query string.
        top_k: Number of top results to return.
        persist_dir: Path to ChromaDB persistence directory.

    Returns:
        List of dicts with keys: 'text', 'source', 'distance'.
    """
    try:
        import chromadb
        from chromadb.utils import embedding_functions
    except ImportError:
        return []

    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    persist_path = os.path.join(project_root, persist_dir)

    if not os.path.isdir(persist_path):
        return []

    client = chromadb.PersistentClient(path=persist_path)

    collection_name = "phase4_docs"
    existing = [c.name for c in client.list_collections()]
    if collection_name not in existing:
        return []

    ef = embedding_functions.SentenceTransformerEmbeddingFunction(
        model_name="all-MiniLM-L6-v2"
    )

    collection = client.get_collection(
        name=collection_name,
        embedding_function=ef,
    )

    results = collection.query(
        query_texts=[question],
        n_results=min(top_k, collection.count()),
        include=["documents", "metadatas", "distances"],
    )

    enriched = []
    if results and results.get("documents"):
        docs = results["documents"][0]
        metas = results.get("metadatas", [[]])[0]
        dists = results.get("distances", [[]])[0]

        for i, doc in enumerate(docs):
            enriched.append({
                "text": doc,
                "source": metas[i].get("source", "unknown") if i < len(metas) else "unknown",
                "distance": dists[i] if i < len(dists) else None,
            })

    return enriched


if __name__ == "__main__":
    # Quick test: query from command line
    if len(sys.argv) > 1:
        query = " ".join(sys.argv[1:])
    else:
        query = "What is the temperature threshold?"

    print(f"Query: {query}\n")
    results = query_rag_with_metadata(query)

    if not results:
        print("No results found. Make sure to run rag_setup.py first.")
    else:
        for i, r in enumerate(results):
            print(f"--- Result {i + 1} (source: {r['source']}, "
                  f"distance: {r['distance']:.4f}) ---")
            print(r["text"][:300])
            print()
