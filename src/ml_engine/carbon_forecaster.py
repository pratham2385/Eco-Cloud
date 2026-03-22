import pandas as pd
import numpy as np
import json
from statsmodels.tsa.arima.model import ARIMA
import warnings

# Ignore statistical warnings for cleaner CLI output
warnings.filterwarnings("ignore")

def train_and_forecast():
    print("   ECO-CLOUD: ML FORECASTING & LOGICAL AGENT")
    
    csv_path = '../../data/raw_carbon_history.csv'
    try:
        df = pd.read_csv(csv_path)
    except FileNotFoundError:
        print("Error: raw_carbon_history.csv not found. Run data_loader.py first.")
        return

    print(">>> Training ARIMA Time-Series Model on 7-day history...")
    
    # Train ARIMA Model (Order 2,0,2 is a good baseline for smooth oscillating data)
    model = ARIMA(df['Carbon_Intensity'], order=(2, 0, 2))
    model_fit = model.fit()
    
    print(">>> Predicting Carbon Intensity for the next 24 hours...")
    # Forecast the next 24 steps (hours)
    forecast = model_fit.forecast(steps=24)
    
    # AI LOGICAL AGENT (Unit IV - Rule Based Thresholding)
    # Calculate the median of the forecast. 
    # Rule: If below median -> GREEN. If above -> DIRTY.
    threshold = np.median(forecast)
    print(f">>> Logical Agent calculated GREEN threshold at: {threshold:.2f} gCO2/kWh")
    
    policy_dict = {}
    
    for i, value in enumerate(forecast):
        state = "GREEN" if value <= threshold else "DIRTY"
        
        policy_dict[str(i)] = {
            "predicted_intensity": round(value, 2),
            "grid_state": state
        }
    
    # Export to JSON for the C++ OS Engine to read
    json_path = '../../data/carbon_config.json'
    with open(json_path, 'w') as json_file:
        json.dump(policy_dict, json_file, indent=4)
        
    print(f"\n>>> Success: Forecast and Scheduling Policy saved to {json_path}")
    print(">>> Sample of generated policy:")
    print(json.dumps({k: policy_dict[k] for k in list(policy_dict)[:3]}, indent=4)) # Print first 3

if __name__ == "__main__":
    train_and_forecast()