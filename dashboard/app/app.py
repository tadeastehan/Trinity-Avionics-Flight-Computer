import dash
from dash import html, dcc
import dash_leaflet as dl
import dash_daq as daq
import os
from dotenv import load_dotenv
import json
from dash.dependencies import Input, Output, State

preferences_file = "preferences.json"
preferences_json = {}

button_clicked = False

def load_env_file():
    load_dotenv()

def get_api_key():
    api_key = os.getenv('MAPY_CZ_API_KEY')
    return api_key

def load_preferences():
    if os.path.exists(preferences_file):
        with open(preferences_file, "r", encoding="utf-8") as file:
            return json.load(file)      
    return {"center": [49.2388992, 16.5546350]}

def save_preferences(preferences):
    print("Saving preferences...")
    with open(preferences_file, "w+", encoding="utf-8") as file:
        json.dump(preferences, file, indent=4, ensure_ascii=False)

def get_center():
    return preferences_json["center"]

load_env_file()
preferences_json = load_preferences()
print(f"Loaded preferences: {preferences_json}")

logo = html.Img(
    className="item1",
    id="logo",
    src=r'assets/imgs/logo.png', alt='image'
)

header_text = html.P(
    className="item2",
    id="header_text",
    children=["Telemetry"]
)

rocket_map_pin = dict(
    iconUrl=r'assets/icons/rocket-launch.png',
    iconSize=[40, 40],
    iconAnchor=[20, 40],
)

start_map_pin = dict(
    iconUrl=r'assets/icons/helicopter.png',
    iconSize=[40, 40],
    iconAnchor=[20, 40],
)

map_div = html.Div(
    className="item3",
    children=[
        dl.Map(
            [
                dl.TileLayer(url=f'https://api.mapy.cz/v1/maptiles/aerial/256/{{z}}/{{x}}/{{y}}?apikey={get_api_key()}'),
                dl.Marker(id="start-marker", position=get_center(), children=[dl.Tooltip("Launch")], icon=start_map_pin),
                dl.Marker(id="rocket-marker", position=[49.2388992, 16.5546350], children=[dl.Tooltip("Rocket")], icon=rocket_map_pin),
                dl.Circle(center=[49.2388992, 16.5546350], radius=5, color='rgba(252, 186, 3, 0.5)', id='rocket-radius')
            ],
            id='map',
            center=get_center(),
            zoom=16,
        ),
        html.Button('Set Launch Position', id='set-marker-button'),
        html.Button('Share Rocket Position', id='share-rocket-position'),
        html.Div(id="placeholder")
    ]
)

altitude_graph = html.Div(
    className="item4",
    id = 'control-panel-altitude',
    children=[
        dcc.Graph(id='control-panel-altitude-graph', config={'displayModeBar': False})        
    ]
)

speed = html.Div(
    className="item6",
    id='control-panel-speed',
    children=[
        daq.Gauge(
            id='control-panel-speed-component',
            label={'label':"Acceleration", 'style':{'fontSize':20}},
            min=0,
            max=10,
            showCurrentValue=True,
            value=1,
            size=175,
            units='1 G',
            color='#fec036',
        )
    ],
    n_clicks=0,
)

altitude_number = html.Div(
    id='control-panel-altitude-component-number',
    children=[
        daq.LEDDisplay(
            id='control-panel-altitude-number',
            value='000.00',
            label={'label':"Altitude", 'style':{'fontSize':24}},
            size=70,
            color='#fec036',
            style = {'color':'#black',},
            backgroundColor='#2b2b2b',
        )
    ],
    n_clicks=0,
)

max_altitude_number = html.Div(
    id='control-panel-max-altitude-component-number',
    children=[
        daq.LEDDisplay(
            id='control-panel-max-altitude-number',
            value='000.00',
            label={'label':"Maximum Altitude", 'style':{'fontSize':24}},
            size=70,
            color='#fec036',
            style={'color': '#black'},
            backgroundColor='#2b2b2b',
        )
    ],
    n_clicks=0,
)

stage_button = html.Div(
    className="stage_button_container",
    id="stage_button_component",
    children=[
        html.Button("INIT", className="stage_button", id="stage1"),
        html.Button("READY", className="stage_button", id="stage2"),
        html.Button("ARM", className="stage_button", id="stage3"),
        html.Button("ASCENT", className="stage_button", id="stage4"),
        html.Button("DEPLOY", className="stage_button", id="stage5"),
        html.Button("DESCENT", className="stage_button", id="stage6"),
        html.Button("LANDED", className="stage_button", id="stage7"),
    ],
)

console_log = html.Div(
    children=[
        html.H1("Recieved data", id="recieved_text"),     
        html.Div( className="terminal", children=[html.Pre("", id='console-log',)])
    ]
)

app = dash.Dash(__name__,
    meta_tags=[
        {'name': 'viewport', 'content': 'width=device-width, initial-scale=1.0'}
    ],)

app.layout = html.Div(
    id='root',
    children=[
        html.Link(href='https://fonts.googleapis.com/css?family=Concert One',rel='stylesheet'),
        html.Div(id='hidden-center', style={'display': 'none'}),
        html.Div(id='zoom-display', style={'margin-top': '10px'}),
        dcc.Interval(id='map-update-interval', interval=3000, n_intervals=0),
        dcc.Interval(id='graph-update-interval',interval=300, n_intervals=0),
        html.Div(
            className="grid-container",
            children=[
                logo,
                header_text,
                map_div,
                altitude_graph,
                speed,
                html.Div(className = "item5",id="altitude_item",children= [altitude_number,max_altitude_number]),
                html.Div(className="item8", children=[stage_button]),
                html.Div(className="item7", children=[console_log]),
            ]
        ),
    ]
)

@app.callback(
    Output('start-marker', 'n_clicks'),
    Input('set-marker-button', 'n_clicks')
)
def toggle_button_clicked(n_clicks):
    global button_clicked  # Access the global variable
    if n_clicks:
        button_clicked = True
    return None  # Reset button state after updating marker position

@app.callback(
    Output('start-marker', 'position'),
    Input('map', 'clickData')
)
def update_marker_position(clickData):
    global button_clicked
    if button_clicked:
        button_clicked = False
        if clickData is not None:
            lat = clickData['latlng']['lat']
            lon = clickData['latlng']['lng']
            preferences_json["center"] = [lat, lon]
            save_preferences(preferences_json)
            print(preferences_json)
            return [lat, lon]
    return dash.no_update



if __name__ == '__main__': 
    app.run(debug=True, port=8050)
