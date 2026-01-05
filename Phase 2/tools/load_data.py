#SENSE layer, respondible for loading data does not analyze or modify
from pathlib import Path
import pandas as pd
def load_data(file_path: str) -> pd.DataFrame:
    """ loads operational metrics data frm the CSV file
    parametrs: pd.DataFrame: load operational data
    Raises: FileNotFoundError: if the file does not exist
    ValueError: If the file cannot be read
    """
    path = Path(file_path) #converting string path into a path object
    if not path.exists():#checking if file pat exists
        raise FileNotFoundError(f"file not found: {file_path}")
    try:
        data = pd.read_csv(path) # try loading the CSV file
    except Exception as error:
        raise ValueError(f"failed to load the data: {error}")
    return data #returning the data
