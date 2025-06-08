import os, json, segno, webbrowser, dash
from dash import html, dcc
import dash_leaflet as dl
import dash_daq as daq
from dotenv import load_dotenv
from dash.dependencies import Input, Output, State
import pandas as pd



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
    return {"start_marker_position": [49.2388992, 16.5546350], "style": "aerial", "rocket_position": [49.2388992, 16.5547350]}

def save_preferences(preferences):
    with open(preferences_file, "w+", encoding="utf-8") as file:
        json.dump(preferences, file, indent=4, ensure_ascii=False)

def get_start_marker_position():
    return preferences_json["start_marker_position"]

def get_style():
    return preferences_json["style"]

def get_rocket_position():
    return preferences_json["rocket_position"]

load_env_file()
preferences_json = load_preferences()
print(f"Loaded preferences: {preferences_json}")

logo = html.Img(
    className="item1",
    id="logo",
    src=r'assets/imgs/logo.png', alt='image'
)

# List CSV files in the data folder for dropdown
DATA_DIR = os.path.join(os.path.dirname(__file__), 'data')
file_options = [
    {"label": f, "value": f} for f in os.listdir(DATA_DIR) if f.endswith('.csv')
]

def get_column_options(file):
    if not file:
        return []
    csv_path = os.path.join(DATA_DIR, file)
    try:
        with open(csv_path, 'r', encoding='utf-8') as f:
            header = f.readline().strip().split(',')
        return [
            {"label": col, "value": col}
            for col in header if col.lower() not in ['time [ms]', 'time [delta_ms]']
        ]
    except Exception:
        return []

header_text = html.Div(
    className="item2",
    id="header_text",
    children=[
        html.P("Telemetry", style={"marginBottom": "8px", "fontSize": 50, "color": "#fec036", "textAlign": "left"}),
        html.Div(
            children=[
                dcc.Dropdown(
                    id='file-dropdown',
                    options=file_options,
                    value=file_options[0]['value'] if file_options else None,
                    clearable=False,
                    style={"width": "160px", "fontSize": 15, "height": "32px"}
                ),
                dcc.Dropdown(
                    id='data-column-dropdown',
                    options=get_column_options(file_options[0]['value']) if file_options else [],
                    value=get_column_options(file_options[0]['value'])[0]['value'] if file_options and get_column_options(file_options[0]['value']) else None,
                    clearable=False,
                    style={"width": "160px", "fontSize": 15, "height": "32px"}
                ),
                daq.ToggleSwitch(
                    id='streamed-data',
                    value=False
                ),
            ]
        )
    ]
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
    style={"position": "relative"},
    children=[
        dl.Map(
            [
                dl.TileLayer(id="map-tiles",
                             url=f'https://api.mapy.cz/v1/maptiles/{{get_style()}}/256/{{z}}/{{x}}/{{y}}?apikey={{get_api_key()}}'),
                dl.Marker(id="start-marker", position=get_start_marker_position(), children=[dl.Tooltip("Launch")], icon=start_map_pin),
                dl.Marker(id="rocket-marker", position=get_rocket_position(), children=[dl.Tooltip("Rocket")], icon=rocket_map_pin),
                dl.Circle(center=get_rocket_position(), radius=5, color='rgba(50, 115, 197, 0.5)', id='rocket-radius')
            ],
            id='map',
            center=get_start_marker_position(),
            zoom=16,
        ),
        dcc.Dropdown(
            id='map-style-dropdown',
            options=[
                {'label': 'Aerial', 'value': 'aerial'},
                {'label': 'Basic', 'value': 'basic'},
                {'label': 'Tourist', 'value': 'outdoor'},
            ],
            value='aerial',
            clearable=False,
        ),
        html.Div(
            children=[
            html.Button(
                id='teleport-rocket',
                title='Go to Rocket',
                children=html.Img(src='assets/icons/rocket-outline.svg'),
                n_clicks=0,
            ),
            html.Button(
                id='teleport-home',
                title='Go to Start',
                children=html.Img(src='assets/icons/home-outline.svg'),
                n_clicks=0,
            ),
             html.Button(
                id='set-marker-button',
                title='Set Home Position',
                children=html.Img(src='assets/icons/home-edit-outline.svg'),
                n_clicks=0,
            ),
             html.Button(
                id='share-rocket-position',
                title='Share Rocket Position',
                children=html.Img(src='assets/icons/qrcode.svg'),
                n_clicks=0,
            ),
        ]),
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

battery_indicator = html.Div(
    className="item6",
    id="control-panel-battery",
    children=[
        daq.Tank(
            id="control-panel-battery-component",
            label={"label": "Battery Voltage", "style": {"fontSize": 24}},
            units="V",
            min=6.0,
            max=8.5,
            value=7.8,  # Example value, adjust as needed
            showCurrentValue=True,
            color="#fec036",
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

app._favicon = r'assets/imgs/favicon.ico'
app.title = "Space Force Telemetry"

app.layout = html.Div(
    id='root',
    children=[
        html.Link(href='https://fonts.googleapis.com/css?family=Concert One',rel='stylesheet'),
        html.Div(id='zoom-display', style={'margin-top': '10px'}),
        dcc.Interval(id='map-update-interval', interval=3000, n_intervals=0),
        dcc.Interval(id='slow-update-interval', interval=20*60*1000, n_intervals=0),  # 20 minutes
        dcc.Interval(id='fast-update-interval', interval=1000, n_intervals=0),  # 1 second
        html.Div(
            className="grid-container",
            children=[
                logo,
                header_text,
                map_div,
                altitude_graph,
                battery_indicator,
                html.Div(className = "item5",id="altitude_item",children= [altitude_number]),
                html.Div(className="item9", id="data_column_item", children=[max_altitude_number]),
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
            preferences_json["start_marker_position"] = [lat, lon]
            save_preferences(preferences_json)
            return [lat, lon]
    return dash.no_update

@app.callback(
    Output('map-tiles', 'url'),
    Input('map-style-dropdown', 'value')
)
def update_map_style(value):
    if value is not None:
        preferences_json["style"] = value
        save_preferences(preferences_json)
    return f'https://api.mapy.cz/v1/maptiles/{value}/256/{{z}}/{{x}}/{{y}}?apikey={get_api_key()}'

@app.callback(
    Output("placeholder", "children"),
    [Input("share-rocket-position", "n_clicks")], prevent_initial_call=True
)
def open_qr_code_tab(n_clicks):
    if n_clicks:
        lat, lon = get_rocket_position()

        url = f"https://mapy.cz/letecka?q={lat},{lon}&z=20"
        qr_code = segno.make_qr(url)
        qr_code.save("assets/qr_code.png", scale=10)

        webbrowser.open_new_tab("http://127.0.0.1:8050/assets/qr_code.png")
    return None

# Dynamically update column dropdown when file changes
@app.callback(
    [Output('data-column-dropdown', 'options'), Output('data-column-dropdown', 'value')],
    [Input('file-dropdown', 'value')]
)
def update_column_options(selected_file):
    options = get_column_options(selected_file)
    value = options[0]['value'] if options else None
    return options, value

# Combined callback for file and column dropdowns
@app.callback(
    Output('control-panel-altitude-graph', 'figure'),
    [Input('file-dropdown', 'value'), Input('data-column-dropdown', 'value')]
)
def update_graph(selected_file, selected_column):
    try:
        if not selected_file or not selected_column:
            return {}
        csv_path = os.path.join(DATA_DIR, selected_file)
        df = pd.read_csv(csv_path)
        # Use timestamp/time column if available
        x_col = None
        for col in df.columns:
            if col.lower() in ['time [ms]', 'time [delta_ms]', 'timestamp', 'time']:
                x_col = col
                break
        if not x_col:
            x_col = df.columns[0]
        if selected_column not in df.columns:
            return {}
        fig = {
            'data': [
                {'x': df[x_col] / 1000 if 'ms' in x_col.lower() else df[x_col], 'y': df[selected_column], 'type': 'line', 'name': selected_column}
            ],
            'layout': {
                'title': selected_column,
                'xaxis': {'title': x_col.replace('ms', 's').replace('MS', 's').replace('Ms', 's') if 'ms' in x_col.lower() else x_col},
                'yaxis': {'title': selected_column},
                'plot_bgcolor': '#2b2b2b',
                'paper_bgcolor': '#2b2b2b',
                'font': {'color': '#fec036'}
            }
        }
        return fig
    except Exception as e:
        return {}

@app.callback(
    [Output('control-panel-altitude-number', 'value'),
     Output('control-panel-max-altitude-number', 'value'),
     Output('control-panel-altitude-number', 'label'),
     Output('control-panel-max-altitude-number', 'label')],
    [Input('file-dropdown', 'value'), Input('data-column-dropdown', 'value')]
)
def update_altitude_numbers(selected_file, selected_column):
    try:
        if not selected_file or not selected_column:
            return '000.00', '000.00', {'label': 'Value', 'style': {'fontSize': 24}}, {'label': 'Maximum Value', 'style': {'fontSize': 24}}
        csv_path = os.path.join(DATA_DIR, selected_file)
        df = pd.read_csv(csv_path)
        if selected_column in df.columns:
            min_val = df[selected_column].min()
            max_val = df[selected_column].max()
            label = selected_column.replace('_', ' ')
            # If the selected column is pressure, use smaller font size
            min_val_string = f"{min_val:.2f}"
            max_val_string = f"{max_val:.2f}"
            label_style = {'fontSize': 24}
            if 'pressure' in selected_column.lower():
                min_val_string = f"{min_val:.0f}"
                max_val_string = f"{max_val:.0f}"
            elif 'altitude' in selected_column.lower():
                min_val_string = f"{min_val:.0f}"
                max_val_string = f"{max_val:.0f}"
                
            return f"{min_val_string}", f"{max_val_string}", {'label': label, 'style': label_style}, {'label': f"Maximum {label}", 'style': label_style}
        else:
            return '000.00', '000.00', {'label': 'Value', 'style': {'fontSize': 24}}, {'label': 'Maximum Value', 'style': {'fontSize': 24}}
    except Exception:
        return '000.00', '000.00', {'label': 'Value', 'style': {'fontSize': 24}}, {'label': 'Maximum Value', 'style': {'fontSize': 24}}

@app.callback(
    Output('map', 'children'),
    [Input('file-dropdown', 'value')]
)
def update_map_points(selected_file):
    try:
        if not selected_file:
            return dash.no_update
        csv_path = os.path.join(DATA_DIR, selected_file)
        df = pd.read_csv(csv_path)
        lat_col = None
        lon_col = None
        time_col = None
        for col in df.columns:
            if 'latitude' in col.lower():
                lat_col = col
            if 'longitude' in col.lower():
                lon_col = col
            if col.lower() in ['time [ms]', 'time [delta_ms]', 'timestamp', 'time']:
                time_col = col
        points = []
        if lat_col and lon_col and time_col:
            valid = df[(df[lat_col] != 0) & (df[lon_col] != 0)].copy()
            # Calculate frequency (count) for each unique (lat, lon)
            freq_df = valid.groupby([lat_col, lon_col], as_index=False).size()
            freq_df.rename(columns={'size': 'freq'}, inplace=True)
            # Get the latest timestamp for each (lat, lon)
            latest_time = valid.groupby([lat_col, lon_col], as_index=False)[time_col].max()
            merged = pd.merge(freq_df, latest_time, on=[lat_col, lon_col])
            freq_min = merged['freq'].min()
            freq_max = merged['freq'].max()
            t_min = merged[time_col].min()
            # Find the first timestamp where the latitude stays the same for the rest of the data
            last_lat = valid[lat_col].iloc[-1]
            last_lon = valid[lon_col].iloc[-1]
            # Find the first occurrence of this last position
            mask = (valid[lat_col] == last_lat) & (valid[lon_col] == last_lon)
            idx_first = mask.idxmax() if mask.any() else valid.index[-1]
            t_max = valid.loc[idx_first, time_col]

            def time_to_color(t):
                # 5-color gradient: red → orange → yellow → light green → green
                colors = [
                    (255, 0, 0),      # red
                    (255, 167, 0),    # orange
                    (255, 244, 0),    # yellow
                    (163, 255, 0),    # light green
                    (44, 186, 0)      # green
                ]
                if t_max == t_min:
                    return 'rgb(255,0,0)'
                ratio = (t - t_min) / (t_max - t_min)
                ratio = max(0, min(ratio, 1))  # Clamp between 0 and 1
                n = len(colors) - 1
                seg = min(int(ratio * n), n - 1)
                seg_start = seg / n
                seg_end = (seg + 1) / n
                local_ratio = (ratio - seg_start) / (seg_end - seg_start)
                r1, g1, b1 = colors[seg]
                r2, g2, b2 = colors[seg + 1]
                r = int(r1 + (r2 - r1) * local_ratio)
                g = int(g1 + (g2 - g1) * local_ratio)
                b = int(b1 + (b2 - b1) * local_ratio)
                return f'rgb({r},{g},{b})'

            # Find the last valid position for the rocket marker
            if not valid.empty:
                last_valid_lat = valid[lat_col].iloc[-1]
                last_valid_lon = valid[lon_col].iloc[-1]
                rocket_marker_position = [last_valid_lat, last_valid_lon]
            else:
                rocket_marker_position = get_rocket_position()

            points = [
                dl.CircleMarker(
                    center=[row[lat_col], row[lon_col]],
                    radius=5 + 10 * ((row['freq'] - freq_min) / (freq_max - freq_min)) if freq_max > freq_min else 10,
                    color=time_to_color(row[time_col]),
                    fill=True,
                    fillOpacity=0.8,
                    fillColor=time_to_color(row[time_col])
                )
                for _, row in merged.sort_values(time_col).iterrows()
            ]
            # Recreate the map with points and existing layers
            preferences_json["rocket_position"] = rocket_marker_position
            
            return [
                dl.TileLayer(id="map-tiles",
                             url=f'https://api.mapy.cz/v1/maptiles/{{get_style()}}/256/{{z}}/{{x}}/{{y}}?apikey={{get_api_key()}}'),
                dl.Marker(id="start-marker", position=get_start_marker_position(), children=[dl.Tooltip("Launch")], icon=start_map_pin),
                dl.Marker(id="rocket-marker", position=rocket_marker_position, children=[dl.Tooltip("Rocket")], icon=rocket_map_pin),
                dl.Circle(center=rocket_marker_position, radius=5, color='rgba(50, 115, 197, 0.5)', id='rocket-radius'),
            ] + points
        elif lat_col and lon_col:
            points = [
                dl.Marker(position=[row[lat_col], row[lon_col]])
                for _, row in df.iterrows()
                if row[lat_col] != 0 and row[lon_col] != 0
            ]
        # Recreate the map with points and existing layers
        return [
            dl.TileLayer(id="map-tiles",
                         url=f'https://api.mapy.cz/v1/maptiles/{{get_style()}}/256/{{z}}/{{x}}/{{y}}?apikey={{get_api_key()}}'),
            dl.Marker(id="start-marker", position=get_start_marker_position(), children=[dl.Tooltip("Launch")], icon=start_map_pin),
            dl.Marker(id="rocket-marker", position=rocket_marker_position, children=[dl.Tooltip("Rocket")], icon=rocket_map_pin),
            dl.Circle(center=rocket_marker_position, radius=5, color='rgba(252, 186, 3, 0.5)', id='rocket-radius'),
        ] + points
    except Exception:
        return dash.no_update

@app.callback(
    Output('map', 'center'),
    [Input('teleport-rocket', 'n_clicks'), Input('teleport-home', 'n_clicks')],
    [State('map', 'center')]
)
def teleport_map(n_rocket, n_home, current_center):
    ctx = dash.callback_context
    if not ctx.triggered:
        return current_center
    button_id = ctx.triggered[0]['prop_id'].split('.')[0]
    if button_id == 'teleport-rocket':
        # Get rocket position and update preferences
        pos = get_rocket_position()
        return pos
    elif button_id == 'teleport-home':
        # Get start-marker position and update preferences
        pos = get_start_marker_position()
        return pos
    return current_center

@app.callback(
    [Output(f'stage{i+1}', 'style') for i in range(7)],
    [Input('file-dropdown', 'value')]
)
def update_stage_colors(selected_file):
    stages = ["blanchedalmond"] * 7  # startup colors
    stage_colors = [
        "rgb(255, 153, 0)", "rgb(255, 200, 0)", "rgb(255, 224, 0)",
        "rgb(255, 247, 0)", "rgb(184, 245, 0)", "rgb(149, 226, 20)", "rgb(114, 206, 39)"
    ]
    if selected_file:
        csv_path = os.path.join(DATA_DIR, selected_file)
        try:
            df = pd.read_csv(csv_path)
            if 'State[x]' in df.columns:
                stage_number = int(df['State[x]'].iloc[-1])
                if 1 <= stage_number <= 7:
                    stages[:stage_number] = stage_colors[:stage_number]
        except Exception:
            pass
    return tuple({"background-color": color} for color in stages)

if __name__ == '__main__': 
    app.run(debug=True, port=8050)
