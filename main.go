package main

import (
	"fmt"
	"log"
	"net/http"

	"encoding/json"

	"github.com/labstack/echo"
	"github.com/labstack/echo/middleware"
)

// Handler --
type Handler struct {
	Store []Audio
}

// Audio --
type Audio struct {
	F0        float64 `json:"f0"`
	RMS       float64 `json:"rms"`
	Centroid  float64 `json:"centroid"`
	StartedAt string  `json:"startedAt"`
	EndedAt   string  `json:"endedAt"`
	LocalTime int64   `json:"localTime"`
	BPM       string  `json:"bpm"`
}

// Chart --
type Chart struct {
	Labels   []string  `json:"labels"`
	Datasets []Dataset `json:"datasets"`
}

// Dataset --
type Dataset struct {
	Label                     string    `json:"label"`
	Fill                      bool      `json:"fill"`
	LineTension               float64   `json:"lineTension"`
	BackgroundColor           string    `json:"backgroundColor"`
	BorderColor               string    `json:"borderColor"`
	BorderCapStyle            string    `json:"borderCapStyle"`
	BorderDash                []string  `json:"borderDash"`
	BorderDashOffset          float64   `json:"borderDashOffset"`
	BorderJoinStyle           string    `json:"borderJoinStyle"`
	PointBorderColor          string    `json:"pointBorderColor"`
	PointBackgroundColor      string    `json:"pointBackgroundColor"`
	PointBorderWidth          int       `json:"pointBorderWidth"`
	PointHoverRadius          int       `json:"pointHoverRadius"`
	PointHoverBackgroundColor string    `json:"pointHoverBackgroundColor"`
	PointHoverBorderWidth     int       `json:"pointHoverBorderWidth"`
	PointHoverBorderColor     string    `json:"pointHoverBorderColor"`
	PointRadius               int       `json:"pointRadius"`
	PointHitRadius            int       `json:"pointHitRadius"`
	Data                      []float64 `json:"data"`
}

// Receive --
func (h *Handler) Receive(c echo.Context) error {
	var audio Audio
	defer c.Request().Body.Close()
	if err := json.NewDecoder(c.Request().Body).Decode(&audio); err != nil {
		log.Println(err)
	}
	fmt.Printf("%v\n", audio)
	h.Store = append(h.Store, audio)
	return c.NoContent(http.StatusOK)
}

// GetStore --
func (h *Handler) GetStore(c echo.Context) error {
	return c.JSON(http.StatusOK, h.Store)
}

var colors = []string{
	"rgb(75, 192, 192)",  // "green":
	"rgb(54, 162, 235)",  // "blue":
	"rgb(153, 102, 255)", // "purple":
	"rgb(231,233,237)",   // "grey":
	"rgb(255, 99, 132)",  // "red":
	"rgb(255, 159, 64)",  //"orange":
	"rgb(255, 205, 86)",  // "yellow":
}

// NewChart --
func NewChart(props []string) *Chart {
	chart := &Chart{Labels: []string{}, Datasets: []Dataset{}}
	for idx, prop := range props {
		chart.Datasets = append(chart.Datasets, Dataset{})
		chart.Datasets[idx].Label = prop
		chart.Datasets[idx].Fill = false
		chart.Datasets[idx].LineTension = 0.1
		chart.Datasets[idx].BackgroundColor = colors[idx] // h"rgba(75,192,192,0.4)"
		chart.Datasets[idx].BorderColor = colors[idx]     // "rgba(75,192,192,1)"
		chart.Datasets[idx].BorderCapStyle = "butt"
		chart.Datasets[idx].BorderDash = []string{}
		chart.Datasets[idx].BorderDashOffset = 0.0
		chart.Datasets[idx].BorderJoinStyle = "miter"
		chart.Datasets[idx].PointBorderColor = colors[idx] // "rgba(75,192,192,1)"
		chart.Datasets[idx].PointBackgroundColor = "#fff"
		chart.Datasets[idx].PointBorderWidth = 1
		chart.Datasets[idx].PointHoverRadius = 5
		chart.Datasets[idx].PointHoverBackgroundColor = colors[idx] // h"rgba(75,192,192,1)"
		chart.Datasets[idx].PointHoverBorderColor = "rgba(220,220,220,1)"
		chart.Datasets[idx].PointHoverBorderWidth = 2
		chart.Datasets[idx].PointRadius = 1
		chart.Datasets[idx].PointHitRadius = 10
	}
	return chart
}

// GetChart --
func (h *Handler) GetChart(c echo.Context) error {
	chart := NewChart([]string{"f0", "rms", "centroid"})
	for _, audio := range h.Store {
		chart.Labels = append(chart.Labels, audio.StartedAt)
		chart.Datasets[0].Data = append(chart.Datasets[0].Data, audio.F0)
		chart.Datasets[1].Data = append(chart.Datasets[1].Data, audio.RMS)
		chart.Datasets[2].Data = append(chart.Datasets[2].Data, audio.Centroid)
	}
	return c.JSON(http.StatusOK, chart)
}

func main() {
	e := echo.New()
	e.Use(middleware.CORS())
	h := &Handler{}

	e.GET("/api/audio", h.GetStore)
	e.POST("/api/audio", h.Receive)
	e.GET("/api/audio/chart", h.GetChart)
	e.Start(":9091")
}
