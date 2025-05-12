import dash
from dash import html
import dash_leaflet as dl


import os
from dotenv import load_dotenv


load_dotenv()

API_KEY = os.getenv('MAPY_CZ_API_KEY')

app = dash.Dash(__name__)

app.layout = html.Div([
    dl.Map([
        dl.TileLayer(url=f'https://api.mapy.cz/v1/maptiles/aerial/256/{{z}}/{{x}}/{{y}}?apikey={API_KEY}')
    ],
    id='map',
    center=[49.2388992, 16.5546350],
    zoom=16,
    style={'width': '100vw', 'height': '100vh', 'margin': "auto", 'display': 'block'}
    )
])

if __name__ == '__main__':
    app.run(debug=True)
