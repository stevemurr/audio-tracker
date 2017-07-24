import React from 'react';
import {Line} from 'react-chartjs-2';

const baseURI = "http://localhost:9091/api/audio"
// Calculate the mean of an array
function mean(array) {
  var total = 0;
  for (var idx = 0; idx < array.length; idx++) {
    total += array[idx];
  }
  return total / array.length;
}

class App extends React.Component {
  constructor(props) {
    super(props);
    this.state = {
      data: {}
    }
  }

  tick() {
    fetch(baseURI + "/chart").then((res) => {
      return res.json();
    }).then((json) => {
      var a = json.datasets[0].data;
      var b = json.datasets[1].data;
      var c = json.datasets[2].data;
      this.setState({
        averageF0: mean(a).toFixed(2),
        averageRMS: mean(b).toFixed(2),
        averageCentroid: mean(c).toFixed(2)
      })
      this.setState({data: json})
    }).catch((err) => {
      console.log(err);
    })
  }
  componentDidMount() {
    this.interval = setInterval(() => this.tick(), 1500);
  }

  componentWillUnmount() {
    clearInterval(this.interval);
  }
  render() {
    return (
      <div className="columns">
        <div className="column is-three-quarters">
          <section className="hero">
            <div className="hero-body">
              <div className="container">
                <h1 className="title">
                  Chart
                </h1>
              </div>
            </div>
          </section>
          <Line data={this.state.data}/>
        </div>
        <div className="column">
          <section className="hero">
            <div className="hero-body">
              <div className="container">
                <h1 className="title">
                  //
                </h1>
              </div>
            </div>
          </section>
          <Averages
            averageRMS={this.state.averageRMS}
            averageCentroid={this.state.averageCentroid}
            averageF0={this.state.averageF0}/>
        </div>
      </div>
    );
  }
};

class Averages extends React.Component {
  render() {
    return (
      <div>
        <nav className="panel">
          <p className="panel-heading">
            Averages
          </p>
          <a className="panel-block is-active">
            F0: {this.props.averageF0}
          </a>

          <a className="panel-block is-active">
            RMS: {this.props.averageRMS}
          </a>

          <a className="panel-block is-active">
            Centroid: {this.props.averageCentroid}
          </a>
        </nav>
      </div>
    )
  }
}

export default App;
